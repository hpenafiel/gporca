//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal Inc.
//
//	@filename:
//		CXformSplitDQA.cpp
//
//	@doc:
//		Implementation of the splitting of an aggregate into a three levels -- namely,
//		local, intermediate and global, aggregate
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CColumnFactory.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/COperator.h"
#include "gpopt/xforms/CXformSplitDQA.h"
#include "gpopt/xforms/CXformUtils.h"
#include "gpopt/translate/CTranslatorDXLToExpr.h"
#include "naucrates/md/IMDAggregate.h"

using namespace gpmd;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::CXformSplitDQA
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformSplitDQA::CXformSplitDQA
	(
	IMemoryPool *memory_pool
	)
	:
	CXformExploration
		(
		 // pattern
		GPOS_NEW(memory_pool) CExpression
					(
					memory_pool,
					GPOS_NEW(memory_pool) CLogicalGbAgg(memory_pool),
					GPOS_NEW(memory_pool) CExpression(memory_pool, GPOS_NEW(memory_pool) CPatternLeaf(memory_pool)), // relational child
					GPOS_NEW(memory_pool) CExpression(memory_pool, GPOS_NEW(memory_pool) CPatternTree(memory_pool))  // scalar project list
					)
		)
{}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle;
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformSplitDQA::Exfp
	(
	CExpressionHandle &exprhdl
	)
	const
{
	// do not split aggregate if it is not a global aggregate,  has no distinct aggs, has MDQAs, has outer references,
	// or return types of Agg functions are ambiguous
	if (!CLogicalGbAgg::PopConvert(exprhdl.Pop())->FGlobal() ||
		0 == exprhdl.Pdpscalar(1 /*ulChildIndex*/)->UlDistinctAggs() ||
		exprhdl.Pdpscalar(1 /*ulChildIndex*/)->FHasMultipleDistinctAggs() ||
		0 < CDrvdPropRelational::Pdprel(exprhdl.Pdp())->PcrsOuter()->Size() ||
		CXformUtils::FHasAmbiguousType(exprhdl.PexprScalarChild(1 /*ulChildIndex*/), COptCtxt::PoctxtFromTLS()->Pmda())
		)
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::Transform
//
//	@doc:
//		Actual transformation to expand a global aggregate into local,
//		intermediate and global aggregates
//
//---------------------------------------------------------------------------
void
CXformSplitDQA::Transform
	(
	CXformContext *pxfctxt,
	CXformResult *pxfres,
	CExpression *pexpr
	)
	const
{
	GPOS_ASSERT(NULL != pxfctxt);
	GPOS_ASSERT(NULL != pxfres);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	IMemoryPool *memory_pool = pxfctxt->Pmp();

	// extract components
	CExpression *pexprRelational = (*pexpr)[0];
	CExpression *pexprProjectList = (*pexpr)[1];

	HMExprCr *phmexprcr = GPOS_NEW(memory_pool) HMExprCr(memory_pool);
	DrgPexpr *pdrgpexprChildPrEl = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	DrgPcr *pdrgpcrArgDQA = NULL;

	ExtractDistinctCols
				(
				memory_pool,
				pcf,
				md_accessor,
				pexprProjectList,
				pdrgpexprChildPrEl,
				phmexprcr,
				&pdrgpcrArgDQA
				);

	if (NULL == pdrgpcrArgDQA)
	{
		// failed to extract distinct columns
		phmexprcr->Release();
		pdrgpexprChildPrEl->Release();

		return;
	}


	if (0 < pdrgpexprChildPrEl->Size())
	{
		pexprRelational->AddRef();

		// computed columns referred to in the DQA
		CExpression *pexprChildProject = CUtils::PexprLogicalProject
													(
													memory_pool,
													pexprRelational,
													GPOS_NEW(memory_pool) CExpression
																(
																memory_pool,
																GPOS_NEW(memory_pool) CScalarProjectList(memory_pool),
																pdrgpexprChildPrEl
																),
													true /*fNewComputedCol*/
													);
		pexprRelational = pexprChildProject;
	}

	// multi-stage for both scalar and non-scalar aggregates.
	CExpression *pexprAlt1 = PexprSplitHelper
								(
								memory_pool,
								pcf,
								md_accessor,
								pexpr,
								pexprRelational,
								phmexprcr,
								pdrgpcrArgDQA,
								false /*fSpillTo2Level*/
								);
        
	pxfres->Add(pexprAlt1);

	DrgPcr *pDrgPcr = CLogicalGbAgg::PopConvert(pexpr->Pop())->Pdrgpcr();
	BOOL fScalarDQA = (pDrgPcr == NULL || pDrgPcr->Size() == 0);
	BOOL fForce3StageScalarDQA = GPOS_FTRACE(EopttraceForceThreeStageScalarDQA);
	if (!(fForce3StageScalarDQA && fScalarDQA)) {
		// we skip this option if it is a Scalar DQA and we only want plans with 3-stages of aggregation

		// local/global for both scalar and non-scalar aggregates.
		CExpression *pexprAlt2 = PexprSplitIntoLocalDQAGlobalAgg
				(
				memory_pool,
				pcf,
				md_accessor,
				pexpr,
				pexprRelational,
				phmexprcr,
				pdrgpcrArgDQA
				);

		pxfres->Add(pexprAlt2);
	}

	if (fScalarDQA && !fForce3StageScalarDQA) {
		// if only want 3-stage DQA then skip this 2-stage option for scalar DQA.

		// special case for 'scalar DQA' only, transform to 2-stage aggregate.
		// It's beneficial for distinct column same as distributed column.
		CExpression *pexprAlt3 = PexprSplitHelper
				(
				memory_pool,
				pcf,
				md_accessor,
				pexpr,
				pexprRelational,
				phmexprcr,
				pdrgpcrArgDQA,
				true /*fSpillTo2Level*/
				);
		pxfres->Add(pexprAlt3);
	}
        
	pdrgpcrArgDQA->Release();

	// clean up
	if (0 < pdrgpexprChildPrEl->Size())
	{
		pexprRelational->Release();
	}
	else
	{
		pdrgpexprChildPrEl->Release();
	}

	phmexprcr->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::PexprSplitIntoLocalDQAGlobalAgg
//
//	@doc:
// 		Split the DQA into a local DQA and global agg function
//
//---------------------------------------------------------------------------
CExpression *
CXformSplitDQA::PexprSplitIntoLocalDQAGlobalAgg
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	CMDAccessor *md_accessor,
	CExpression *pexpr,
	CExpression *pexprRelational,
	HMExprCr *phmexprcr,
	DrgPcr *pdrgpcrArgDQA
	)
{
	CExpression *pexprPrL = (*pexpr)[1];

	CLogicalGbAgg *popAgg = CLogicalGbAgg::PopConvert(pexpr->Pop());
	DrgPcr *pdrgpcrGlobal = popAgg->Pdrgpcr();

	// array of project elements for the local, intermediate and global aggregate operator
	DrgPexpr *pdrgpexprPrElFirstStage = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	DrgPexpr *pdrgpexprPrElLastStage = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);

	const ULONG arity = pexprPrL->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprPrEl = (*pexprPrL)[ul];
		CScalarProjectElement *popScPrEl = CScalarProjectElement::PopConvert(pexprPrEl->Pop());

		// get the scalar aggregate function
		CExpression *pexprAggFunc = (*pexprPrEl)[0];
		CScalarAggFunc *popScAggFunc = CScalarAggFunc::PopConvert(pexprAggFunc->Pop());

		if (popScAggFunc->IsDistinct())
		{
			// create a new local DQA version of the original global DQA
			popScAggFunc->MDId()->AddRef();
			CScalarAggFunc *popScAggFuncLocal = CUtils::PopAggFunc
												(
												memory_pool,
												popScAggFunc->MDId(),
												GPOS_NEW(memory_pool) CWStringConst(memory_pool, popScAggFunc->PstrAggFunc()->GetBuffer()),
												true /* is_distinct */,
												EaggfuncstageLocal /*eaggfuncstage*/,
												true /* fSplit */
												);

			GPOS_ASSERT(1 == pexprAggFunc->Arity());
			CExpression *pexprArg = (*pexprAggFunc)[0];
			CColRef *pcrDistinctCol = phmexprcr->Find(pexprArg);
			GPOS_ASSERT(NULL != pcrDistinctCol);
			DrgPexpr *pdrgpexprArgsLocal = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
			pdrgpexprArgsLocal->Append(CUtils::PexprScalarIdent(memory_pool, pcrDistinctCol));

			const IMDAggregate *pmdagg = md_accessor->Pmdagg(popScAggFunc->MDId());
			const IMDType *pmdtype = md_accessor->Pmdtype(pmdagg->PmdidTypeIntermediate());
			CColRef *pcrLocal = pcf->PcrCreate(pmdtype, IDefaultTypeModifier);

			CExpression *pexprPrElLocal = CUtils::PexprScalarProjectElement
													(
													memory_pool,
													pcrLocal,
													GPOS_NEW(memory_pool) CExpression(memory_pool, popScAggFuncLocal, pdrgpexprArgsLocal)
													);

			pdrgpexprPrElFirstStage->Append(pexprPrElLocal);

			// create a new "non-distinct" global aggregate version of the original DQA
			popScAggFunc->MDId()->AddRef();
			CScalarAggFunc *popScAggFuncGlobal = CUtils::PopAggFunc
													(
													memory_pool,
													popScAggFunc->MDId(),
													GPOS_NEW(memory_pool) CWStringConst(memory_pool, popScAggFunc->PstrAggFunc()->GetBuffer()),
													false /* is_distinct */,
													EaggfuncstageGlobal /*eaggfuncstage*/,
													true /* fSplit */
													);

			DrgPexpr *pdrgpexprArgsGlobal = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
			pdrgpexprArgsGlobal->Append(CUtils::PexprScalarIdent(memory_pool, pcrLocal));

			CExpression *pexprPrElGlobal = CUtils::PexprScalarProjectElement
													(
													memory_pool,
													popScPrEl->Pcr(),
													GPOS_NEW(memory_pool) CExpression(memory_pool, popScAggFuncGlobal, pdrgpexprArgsGlobal)
													);

			pdrgpexprPrElLastStage->Append(pexprPrElGlobal);
		}
		else
		{
			// split regular aggregate function into multi-level aggregate functions
			PopulatePrLMultiPhaseAgg
				(
				memory_pool,
				pcf,
				md_accessor,
				pexprPrEl,
				pdrgpexprPrElFirstStage,
				NULL, /* pdrgpexprPrElSecondStage*/
				pdrgpexprPrElLastStage,
				true /* fSplit2LevelsOnly */
				);
		}
	}

	CExpression *pexprGlobal = PexprMultiLevelAggregation
								(
								memory_pool,
								pexprRelational,
								pdrgpexprPrElFirstStage,
								NULL, /* pdrgpexprPrElSecondStage */
								pdrgpexprPrElLastStage,
								pdrgpcrArgDQA,
								pdrgpcrGlobal,
								true /* fSplit2LevelsOnly */,
								false /* fAddDistinctColToLocalGb */
								);

	return pexprGlobal;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::PexprSplitHelper
//
//	@doc:
//		Helper function to Split DQA into 2-stage or 3-stage aggregation.
//
//		If fSpillTo2Level is FALSE, split the 'group by' operator into 3-stage aggregation,
//		e.g., first, second and last aggregates.
//
//		If fSpillTo2Level is TRUE, split the 'group by' operator into 2-stage aggregation,
//		e.g., first, last aggregates. (second aggregate function becomes empty.)
//
//		In both scenarios, add the new aggregate functions to the project list of the
//		corresponding group by operator at each stage of the multi-stage aggregation.
//
//---------------------------------------------------------------------------
CExpression *
CXformSplitDQA::PexprSplitHelper
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	CMDAccessor *md_accessor,
	CExpression *pexpr,
	CExpression *pexprRelational,
	HMExprCr *phmexprcr,
	DrgPcr *pdrgpcrArgDQA,
	BOOL fSpillTo2Level
	)
{
	CExpression *pexprPrL = (*pexpr)[1];

	CLogicalGbAgg *popAgg = CLogicalGbAgg::PopConvert(pexpr->Pop());
	DrgPcr *pdrgpcrGlobal = popAgg->Pdrgpcr();

	// array of project elements for the local (first), intermediate
	// (second) and global (third) aggregate operator
	DrgPexpr *pdrgpexprPrElFirstStage = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	DrgPexpr *pdrgpexprPrElSecondStage = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
	DrgPexpr *pdrgpexprPrElLastStage = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);

	const ULONG arity = pexprPrL->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprPrEl = (*pexprPrL)[ul];
		CScalarProjectElement *popScPrEl = CScalarProjectElement::PopConvert(pexprPrEl->Pop());

		// get the scalar aggregate function
		CExpression *pexprAggFunc = (*pexprPrEl)[0];
		CScalarAggFunc *popScAggFunc = CScalarAggFunc::PopConvert(pexprAggFunc->Pop());

		if (popScAggFunc->IsDistinct())
		{
			// create a new "non-distinct" version of the original aggregate function
			popScAggFunc->MDId()->AddRef();
			CScalarAggFunc *popScAggFuncNew = CUtils::PopAggFunc
												(
												memory_pool,
												popScAggFunc->MDId(),
												GPOS_NEW(memory_pool) CWStringConst(memory_pool, popScAggFunc->PstrAggFunc()->GetBuffer()),
												false /* is_distinct */,
												EaggfuncstageGlobal /*eaggfuncstage*/,
												false /* fSplit */
												);

			GPOS_ASSERT(1 == pexprAggFunc->Arity());
			CExpression *pexprArg = (*pexprAggFunc)[0];

			CColRef *pcrDistinctCol = phmexprcr->Find(pexprArg);
			GPOS_ASSERT(NULL != pcrDistinctCol);
			DrgPexpr *pdrgpexprArgs = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
			pdrgpexprArgs->Append(CUtils::PexprScalarIdent(memory_pool, pcrDistinctCol));

			CExpression *pexprPrElGlobal = CUtils::PexprScalarProjectElement
													(
													memory_pool,
													popScPrEl->Pcr(),
													GPOS_NEW(memory_pool) CExpression(memory_pool, popScAggFuncNew, pdrgpexprArgs)
													);

			pdrgpexprPrElLastStage->Append(pexprPrElGlobal);
		}
		else
		{
			// split the regular aggregate function into multi-level aggregate functions
			PopulatePrLMultiPhaseAgg
				(
				memory_pool,
				pcf,
				md_accessor,
				pexprPrEl,
				pdrgpexprPrElFirstStage,
				pdrgpexprPrElSecondStage,
				pdrgpexprPrElLastStage,
				fSpillTo2Level
				);
		}
	}

	CExpression *pexprGlobal = PexprMultiLevelAggregation
								(
								memory_pool,
								pexprRelational,
								pdrgpexprPrElFirstStage,
								pdrgpexprPrElSecondStage,
								pdrgpexprPrElLastStage,
								pdrgpcrArgDQA,
								pdrgpcrGlobal,
								fSpillTo2Level,
								true /* fAddDistinctColToLocalGb */
								);

	// clean-up the secondStage if spill to 2 level
	if (fSpillTo2Level)
	{
		pdrgpexprPrElSecondStage->Release();
	}

	return pexprGlobal;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::PexprPrElAgg
//
//	@doc:
//		Create an aggregate function  of a particular level and a project
//		project element to hold it
//
//---------------------------------------------------------------------------
CExpression *
CXformSplitDQA::PexprPrElAgg
	(
	IMemoryPool *memory_pool,
	CExpression *pexprAggFunc,
	EAggfuncStage eaggfuncstage,
	CColRef *pcrPreviousStage,
	CColRef *pcrCurrStage
	)
{
	GPOS_ASSERT(NULL != pexprAggFunc);
	GPOS_ASSERT(NULL != pcrCurrStage);
	GPOS_ASSERT(EaggfuncstageSentinel != eaggfuncstage);

	CScalarAggFunc *popScAggFunc = CScalarAggFunc::PopConvert(pexprAggFunc->Pop());
	GPOS_ASSERT(!popScAggFunc->IsDistinct());

	// project element of global aggregation
	DrgPexpr *pdrgpexprArg = NULL;
	if (EaggfuncstageLocal == eaggfuncstage)
	{
		DrgPexpr *pdrgpexprAggOrig = pexprAggFunc->PdrgPexpr();
		pdrgpexprAggOrig->AddRef();
		pdrgpexprArg = pdrgpexprAggOrig;
	}
	else
	{
		pdrgpexprArg = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);
		pdrgpexprArg->Append(CUtils::PexprScalarIdent(memory_pool, pcrPreviousStage));
	}

	popScAggFunc->MDId()->AddRef();
	CScalarAggFunc *popScAggFuncNew = CUtils::PopAggFunc
												(
												memory_pool,
												popScAggFunc->MDId(),
												GPOS_NEW(memory_pool) CWStringConst(memory_pool, popScAggFunc->PstrAggFunc()->GetBuffer()),
												false, /*fdistinct */
												eaggfuncstage,
												true /* fSplit */
												);

	return CUtils::PexprScalarProjectElement
					(
					memory_pool,
					pcrCurrStage,
					GPOS_NEW(memory_pool) CExpression(memory_pool, popScAggFuncNew, pdrgpexprArg)
					);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::PopulatePrLMultiPhaseAgg
//
//	@doc:
// 		Given a scalar aggregate generate the local, intermediate and global
// 		aggregate function. Then add it to the project list of the corresponding
// 		aggregate operator at each stage of the multi-stage aggregation
//
//---------------------------------------------------------------------------
void
CXformSplitDQA::PopulatePrLMultiPhaseAgg
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	CMDAccessor *md_accessor,
	CExpression *pexprPrEl,
	DrgPexpr *pdrgpexprPrElFirstStage,
	DrgPexpr *pdrgpexprPrElSecondStage,
	DrgPexpr *pdrgpexprPrElLastStage,
	BOOL fSplit2LevelsOnly
	)
{
	GPOS_ASSERT(NULL != pexprPrEl);
	GPOS_ASSERT(NULL != pdrgpexprPrElFirstStage);
	GPOS_ASSERT_IMP(NULL == pdrgpexprPrElSecondStage, fSplit2LevelsOnly);
	GPOS_ASSERT(NULL != pdrgpexprPrElLastStage);

	// get the components of the project element (agg func)
	CScalarProjectElement *popScPrEl = CScalarProjectElement::PopConvert(pexprPrEl->Pop());
	CExpression *pexprAggFunc = (*pexprPrEl)[0];
	CScalarAggFunc *popScAggFunc = CScalarAggFunc::PopConvert(pexprAggFunc->Pop());

	const IMDAggregate *pmdagg = md_accessor->Pmdagg(popScAggFunc->MDId());
	const IMDType *pmdtype = md_accessor->Pmdtype(pmdagg->PmdidTypeIntermediate());

	// create new column reference for the first stage (local) project element
	CColRef *pcrLocal = pcf->PcrCreate(pmdtype, IDefaultTypeModifier);

	CExpression *pexprPrElFirstStage = PexprPrElAgg(memory_pool, pexprAggFunc, EaggfuncstageLocal, NULL /*pcrPreviousStage*/, pcrLocal);
	pdrgpexprPrElFirstStage->Append(pexprPrElFirstStage);

	// column reference for the second stage project elements
	CColRef *pcrSecondStage = NULL;
	EAggfuncStage eaggfuncstage = EaggfuncstageIntermediate;
	if (fSplit2LevelsOnly)
	{
		eaggfuncstage = EaggfuncstageGlobal;
		pcrSecondStage = popScPrEl->Pcr();
	}
	else
	{
		// create a new column reference for the second stage (intermediate) project element
		pcrSecondStage = pcf->PcrCreate(pmdtype, IDefaultTypeModifier);
	}

	CExpression *pexprPrElSecondStage = PexprPrElAgg(memory_pool, pexprAggFunc, eaggfuncstage, pcrLocal, pcrSecondStage);
	if (fSplit2LevelsOnly)
	{
		pdrgpexprPrElLastStage->Append(pexprPrElSecondStage);
		return;
	}

	pdrgpexprPrElSecondStage->Append(pexprPrElSecondStage);

	// column reference for the third stage project elements
	CColRef *pcrGlobal = popScPrEl->Pcr();
	CExpression *pexprPrElGlobal = PexprPrElAgg(memory_pool, pexprAggFunc, EaggfuncstageGlobal, pcrSecondStage, pcrGlobal);

	pdrgpexprPrElLastStage->Append(pexprPrElGlobal);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::PcrAggFuncArgument
//
//	@doc:
//		Return the column reference of the argument to the aggregate function.
//		If the argument is a computed column then create a new project element
//		in the child's project list, else just return its column reference
//
//---------------------------------------------------------------------------
CColRef *
CXformSplitDQA::PcrAggFuncArgument
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	CColumnFactory *pcf,
	CExpression *pexprArg,
	DrgPexpr *pdrgpexprChildPrEl
	)
{
	GPOS_ASSERT(NULL != pexprArg);
	GPOS_ASSERT(NULL != pdrgpexprChildPrEl);

	if (COperator::EopScalarIdent == pexprArg->Pop()->Eopid())
	{
		return (const_cast<CColRef *>(CScalarIdent::PopConvert(pexprArg->Pop())->Pcr()));
	}

	CScalar *popScalar = CScalar::PopConvert(pexprArg->Pop());
	// computed argument to the input
	const IMDType *pmdtype = md_accessor->Pmdtype(popScalar->MDIdType());
	CColRef *pcrAdditionalGrpCol = pcf->PcrCreate(pmdtype, popScalar->TypeModifier());

	pexprArg->AddRef();
	CExpression *pexprPrElNew = CUtils::PexprScalarProjectElement(memory_pool, pcrAdditionalGrpCol, pexprArg);

	pdrgpexprChildPrEl->Append(pexprPrElNew);

	return pcrAdditionalGrpCol;
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::PexprMultiLevelAggregation
//
//	@doc:
//		Generate an expression with multi-level aggregation
//
//---------------------------------------------------------------------------
CExpression *
CXformSplitDQA::PexprMultiLevelAggregation
	(
	IMemoryPool *memory_pool,
	CExpression *pexprRelational,
	DrgPexpr *pdrgpexprPrElFirstStage,
	DrgPexpr *pdrgpexprPrElSecondStage,
	DrgPexpr *pdrgpexprPrElThirdStage,
	DrgPcr *pdrgpcrArgDQA,
	DrgPcr *pdrgpcrLastStage,
	BOOL fSplit2LevelsOnly,
	BOOL fAddDistinctColToLocalGb
	)
{
	GPOS_ASSERT(NULL != pexprRelational);
	GPOS_ASSERT(NULL != pdrgpexprPrElFirstStage);
	GPOS_ASSERT(NULL != pdrgpexprPrElThirdStage);
	GPOS_ASSERT(NULL != pdrgpcrArgDQA);

	GPOS_ASSERT_IMP(!fAddDistinctColToLocalGb, fSplit2LevelsOnly);

	DrgPcr *pdrgpcrLocal = CUtils::PdrgpcrExactCopy(memory_pool, pdrgpcrLastStage);
	const ULONG ulLen = pdrgpcrArgDQA->Size();
	GPOS_ASSERT(0 < ulLen);

	if (fAddDistinctColToLocalGb)
	{
		// add the distinct column to the group by at the first stage of
		// the multi-level aggregation
		CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, pdrgpcrLocal);
		for (ULONG ul = 0; ul < ulLen; ul++)
		{
			CColRef *pcr = (*pdrgpcrArgDQA)[ul];
			if (!pcrs->FMember(pcr))
			{
				pdrgpcrLocal->Append(pcr);
				pcrs->Include(pcr);
			}
		}
		pcrs->Release();
	}

	CLogicalGbAgg *popFirstStage = NULL;
	CLogicalGbAgg *popSecondStage = NULL;
	DrgPexpr *pdrgpexprLastStage = pdrgpexprPrElSecondStage;
	if (fSplit2LevelsOnly)
	{
		// for scalar DQA the local aggregate is responsible for removing duplicates
		BOOL fLocalAggGeneratesDuplicates = (0 < pdrgpcrLastStage->Size());

		pdrgpcrArgDQA->AddRef();
		popFirstStage = GPOS_NEW(memory_pool) CLogicalGbAgg
									(
									memory_pool,
									pdrgpcrLocal,
									COperator::EgbaggtypeLocal,
									fLocalAggGeneratesDuplicates,
									pdrgpcrArgDQA
									);
		pdrgpcrLastStage->AddRef();
		popSecondStage = GPOS_NEW(memory_pool) CLogicalGbAgg(memory_pool, pdrgpcrLastStage, COperator::EgbaggtypeGlobal /* egbaggtype */);
		pdrgpexprLastStage = pdrgpexprPrElThirdStage;
	}
	else
	{
		popFirstStage = GPOS_NEW(memory_pool) CLogicalGbAgg(memory_pool, pdrgpcrLocal, COperator::EgbaggtypeLocal /* egbaggtype */);
		pdrgpcrLocal->AddRef();
		pdrgpcrArgDQA->AddRef();
		popSecondStage = GPOS_NEW(memory_pool) CLogicalGbAgg
									(
									memory_pool,
									pdrgpcrLocal,
									COperator::EgbaggtypeIntermediate,
									false, /* fGeneratesDuplicates */
									pdrgpcrArgDQA
									);
	}

	pexprRelational->AddRef();
	CExpression *pexprFirstStage = GPOS_NEW(memory_pool) CExpression
											(
											memory_pool,
											popFirstStage,
											pexprRelational,
											GPOS_NEW(memory_pool) CExpression
														(
														memory_pool,
														GPOS_NEW(memory_pool) CScalarProjectList(memory_pool),
														pdrgpexprPrElFirstStage
														)
											);

	CExpression *pexprSecondStage = GPOS_NEW(memory_pool) CExpression
												(
												memory_pool,
												popSecondStage,
												pexprFirstStage,
												GPOS_NEW(memory_pool) CExpression
															(
															memory_pool,
															GPOS_NEW(memory_pool) CScalarProjectList(memory_pool),
															pdrgpexprLastStage
															)
												);

	if (fSplit2LevelsOnly)
	{
		return pexprSecondStage;
	}

	pdrgpcrLastStage->AddRef();
	return GPOS_NEW(memory_pool) CExpression
						(
						memory_pool,
						GPOS_NEW(memory_pool) CLogicalGbAgg(memory_pool, pdrgpcrLastStage, COperator::EgbaggtypeGlobal /* egbaggtype */),
						pexprSecondStage,
						GPOS_NEW(memory_pool) CExpression
									(
									memory_pool,
									GPOS_NEW(memory_pool) CScalarProjectList(memory_pool),
									pdrgpexprPrElThirdStage
									)
						);
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSplitDQA::ExtractDistinctCols
//
//	@doc:
//		Extract arguments of distinct aggs
//
//---------------------------------------------------------------------------
void
CXformSplitDQA::ExtractDistinctCols
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	CMDAccessor *md_accessor,
	CExpression *pexpr,
	DrgPexpr *pdrgpexprChildPrEl,
	HMExprCr *phmexprcr,
	DrgPcr **ppdrgpcrArgDQA  // output: array of distinct aggs arguments
	)
{
	GPOS_ASSERT(NULL != pdrgpexprChildPrEl);
	GPOS_ASSERT(NULL != ppdrgpcrArgDQA);
	GPOS_ASSERT(NULL != phmexprcr);

	const ULONG arity = pexpr->Arity();

	// use a set to deduplicate distinct aggs arguments
	CColRefSet *pcrsArgDQA = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	ULONG ulDistinct = 0;
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprPrEl = (*pexpr)[ul];

		// get the scalar child of the project element
		CExpression *pexprAggFunc = (*pexprPrEl)[0];
		CScalarAggFunc *popScAggFunc = CScalarAggFunc::PopConvert(pexprAggFunc->Pop());

		if (popScAggFunc->IsDistinct() && md_accessor->Pmdagg(popScAggFunc->MDId())->FSplittable())
		{
			GPOS_ASSERT(1 == pexprAggFunc->Arity());
			
			CExpression *pexprArg = (*pexprAggFunc)[0];
			GPOS_ASSERT(NULL != pexprArg);
			CColRef *pcrDistinctCol = phmexprcr->Find(pexprArg);
			if (NULL == pcrDistinctCol)
			{
				ulDistinct++;

				// get the column reference of the DQA argument
				pcrDistinctCol = PcrAggFuncArgument(memory_pool, md_accessor, pcf, pexprArg, pdrgpexprChildPrEl);

				// insert into the map between the expression representing the DQA argument 
				// and its column reference
				pexprArg->AddRef();
#ifdef GPOS_DEBUG
				BOOL fInserted =
#endif
						phmexprcr->Insert(pexprArg, pcrDistinctCol);
				GPOS_ASSERT(fInserted);

				// add the distinct column to the set of distinct columns
				pcrsArgDQA->Include(pcrDistinctCol);
			}
		}
	}

	if (1 == ulDistinct)
	{
		*ppdrgpcrArgDQA = pcrsArgDQA->Pdrgpcr(memory_pool);
	}
	else
	{
		// failed to find a single DQA, or agg is defined as non-splittable
		*ppdrgpcrArgDQA = NULL;
	}
	pcrsArgDQA->Release();
}

// EOF
