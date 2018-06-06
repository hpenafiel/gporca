//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTranslatorExprToDXL.cpp
//
//	@doc:
//		Implementation of the methods for translating Optimizer physical expression
//		trees into DXL.
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpos/common/CAutoTimer.h"
#include "gpos/common/CHashMap.h"

#include "naucrates/md/IMDCast.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/md/CMDRelationCtasGPDB.h"

#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLWindowFrame.h"
#include "naucrates/dxl/operators/dxlops.h"

#include "naucrates/statistics/CStatistics.h"

#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/exception.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CPhysicalAgg.h"
#include "gpopt/operators/CPhysicalMotionRandom.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/translate/CTranslatorExprToDXL.h"
#include "gpopt/translate/CTranslatorDXLToExpr.h"
#include "gpopt/translate/CTranslatorExprToDXLUtils.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CCastUtils.h"

#include "naucrates/base/IDatumInt8.h"
#include "naucrates/base/CDatumBoolGPDB.h"

#include "naucrates/traceflags/traceflags.h"

using namespace gpos;
using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;
using namespace gpnaucrates;

#define GPOPT_MASTER_SEGMENT_ID (-1)

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::CTranslatorExprToDXL
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CTranslatorExprToDXL::CTranslatorExprToDXL
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	IntPtrArray *pdrgpiSegments,
	BOOL fInitColumnFactory
	)
	:
	m_memory_pool(memory_pool),
	m_pmda(md_accessor),
	m_pdpplan(NULL),
	m_pcf(NULL),
	m_pdrgpiSegments(pdrgpiSegments),
	m_iMasterId(GPOPT_MASTER_SEGMENT_ID)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != md_accessor);
	GPOS_ASSERT_IMP(NULL != pdrgpiSegments, (0 < pdrgpiSegments->Size()));

	InitScalarTranslators();
	InitPhysicalTranslators();

	// initialize hash map
	m_phmcrdxln = GPOS_NEW(m_memory_pool) HMCrDxln(m_memory_pool);

	m_phmcrdxlnIndexLookup = GPOS_NEW(m_memory_pool) HMCrDxln(m_memory_pool);

	if (fInitColumnFactory)
	{
		// get column factory from optimizer context object
		m_pcf = COptCtxt::PoctxtFromTLS()->Pcf();
		GPOS_ASSERT(NULL != m_pcf);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::~CTranslatorExprToDXL
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CTranslatorExprToDXL::~CTranslatorExprToDXL()
{
	CRefCount::SafeRelease(m_pdrgpiSegments);
	m_phmcrdxln->Release();
	m_phmcrdxlnIndexLookup->Release();
	CRefCount::SafeRelease(m_pdpplan);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::InitScalarTranslators
//
//	@doc:
//		Initialize index of scalar translators
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::InitScalarTranslators()
{
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(m_rgpfScalarTranslators); ul++)
	{
		m_rgpfScalarTranslators[ul] = NULL;
	}

	// array mapping operator type to translator function
	SScTranslatorMapping rgScalarTranslators[] = 
	{
			{COperator::EopScalarIdent, &gpopt::CTranslatorExprToDXL::PdxlnScId},
			{COperator::EopScalarCmp, &gpopt::CTranslatorExprToDXL::PdxlnScCmp},
			{COperator::EopScalarIsDistinctFrom, &gpopt::CTranslatorExprToDXL::PdxlnScDistinctCmp},
			{COperator::EopScalarOp, &gpopt::CTranslatorExprToDXL::PdxlnScOp},
			{COperator::EopScalarBoolOp, &gpopt::CTranslatorExprToDXL::PdxlnScBoolExpr},
			{COperator::EopScalarConst, &gpopt::CTranslatorExprToDXL::PdxlnScConst},
			{COperator::EopScalarFunc, &gpopt::CTranslatorExprToDXL::PdxlnScFuncExpr},
			{COperator::EopScalarWindowFunc, &gpopt::CTranslatorExprToDXL::PdxlnScWindowFuncExpr},
			{COperator::EopScalarAggFunc,&gpopt::CTranslatorExprToDXL::PdxlnScAggref},
			{COperator::EopScalarNullIf, &gpopt::CTranslatorExprToDXL::PdxlnScNullIf},
			{COperator::EopScalarNullTest, &gpopt::CTranslatorExprToDXL::PdxlnScNullTest},
			{COperator::EopScalarBooleanTest, &gpopt::CTranslatorExprToDXL::PdxlnScBooleanTest},
			{COperator::EopScalarIf, &gpopt::CTranslatorExprToDXL::PdxlnScIfStmt},
			{COperator::EopScalarSwitch, &gpopt::CTranslatorExprToDXL::PdxlnScSwitch},
			{COperator::EopScalarSwitchCase, &gpopt::CTranslatorExprToDXL::PdxlnScSwitchCase},
			{COperator::EopScalarCaseTest, &gpopt::CTranslatorExprToDXL::PdxlnScCaseTest},
			{COperator::EopScalarCoalesce, &gpopt::CTranslatorExprToDXL::PdxlnScCoalesce},
			{COperator::EopScalarMinMax, &gpopt::CTranslatorExprToDXL::PdxlnScMinMax},
			{COperator::EopScalarCast, &gpopt::CTranslatorExprToDXL::PdxlnScCast},
			{COperator::EopScalarCoerceToDomain, &gpopt::CTranslatorExprToDXL::PdxlnScCoerceToDomain},
			{COperator::EopScalarCoerceViaIO, &gpopt::CTranslatorExprToDXL::PdxlnScCoerceViaIO},
			{COperator::EopScalarArrayCoerceExpr, &gpopt::CTranslatorExprToDXL::PdxlnScArrayCoerceExpr},
			{COperator::EopScalarArray, &gpopt::CTranslatorExprToDXL::PdxlnArray},
			{COperator::EopScalarArrayCmp, &gpopt::CTranslatorExprToDXL::PdxlnArrayCmp},
			{COperator::EopScalarArrayRef, &gpopt::CTranslatorExprToDXL::PdxlnArrayRef},
			{COperator::EopScalarArrayRefIndexList, &gpopt::CTranslatorExprToDXL::PdxlnArrayRefIndexList},
			{COperator::EopScalarAssertConstraintList, &gpopt::CTranslatorExprToDXL::PdxlnAssertPredicate},
			{COperator::EopScalarAssertConstraint, &gpopt::CTranslatorExprToDXL::PdxlnAssertConstraint},
			{COperator::EopScalarDMLAction, &gpopt::CTranslatorExprToDXL::PdxlnDMLAction},
			{COperator::EopScalarBitmapIndexProbe, &gpopt::CTranslatorExprToDXL::PdxlnBitmapIndexProbe},
			{COperator::EopScalarBitmapBoolOp, &gpopt::CTranslatorExprToDXL::PdxlnBitmapBoolOp},
	};

	const ULONG ulTranslators = GPOS_ARRAY_SIZE(rgScalarTranslators);
	for (ULONG ul = 0; ul < ulTranslators; ul++)
	{
		SScTranslatorMapping elem = rgScalarTranslators[ul];
		m_rgpfScalarTranslators[elem.eopid] = elem.pf;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::InitPhysicalTranslators
//
//	@doc:
//		Initialize index of physical translators
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::InitPhysicalTranslators()
{
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(m_rgpfPhysicalTranslators); ul++)
	{
		m_rgpfPhysicalTranslators[ul] = NULL;
	}

	// array mapping operator type to translator function
	SPhTranslatorMapping rgPhysicalTranslators[] =
	{
			{COperator::EopPhysicalFilter, &gpopt::CTranslatorExprToDXL::PdxlnResult},
			{COperator::EopPhysicalIndexScan, &gpopt::CTranslatorExprToDXL::PdxlnIndexScan},
			{COperator::EopPhysicalBitmapTableScan, &gpopt::CTranslatorExprToDXL::PdxlnBitmapTableScan},
			{COperator::EopPhysicalComputeScalar, &gpopt::CTranslatorExprToDXL::PdxlnComputeScalar},
			{COperator::EopPhysicalScalarAgg, &gpopt::CTranslatorExprToDXL::PdxlnAggregate},
			{COperator::EopPhysicalHashAgg, &gpopt::CTranslatorExprToDXL::PdxlnAggregate},
			{COperator::EopPhysicalStreamAgg, &gpopt::CTranslatorExprToDXL::PdxlnAggregate},
			{COperator::EopPhysicalHashAggDeduplicate, &gpopt::CTranslatorExprToDXL::PdxlnAggregateDedup},
			{COperator::EopPhysicalStreamAggDeduplicate, &gpopt::CTranslatorExprToDXL::PdxlnAggregateDedup},
			{COperator::EopPhysicalSort, &gpopt::CTranslatorExprToDXL::PdxlnSort},
			{COperator::EopPhysicalLimit, &gpopt::CTranslatorExprToDXL::PdxlnLimit},
			{COperator::EopPhysicalSequenceProject, &gpopt::CTranslatorExprToDXL::PdxlnWindow},
			{COperator::EopPhysicalInnerNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalInnerIndexNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalLeftOuterIndexNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalCorrelatedInnerNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnCorrelatedNLJoin},
			{COperator::EopPhysicalLeftOuterNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalCorrelatedLeftOuterNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnCorrelatedNLJoin},
			{COperator::EopPhysicalCorrelatedLeftSemiNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnCorrelatedNLJoin},
			{COperator::EopPhysicalCorrelatedInLeftSemiNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnCorrelatedNLJoin},
			{COperator::EopPhysicalLeftSemiNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalLeftAntiSemiNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalLeftAntiSemiNLJoinNotIn, &gpopt::CTranslatorExprToDXL::PdxlnNLJoin},
			{COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnCorrelatedNLJoin},
			{COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin, &gpopt::CTranslatorExprToDXL::PdxlnCorrelatedNLJoin},
			{COperator::EopPhysicalInnerHashJoin, &gpopt::CTranslatorExprToDXL::PdxlnHashJoin},
			{COperator::EopPhysicalLeftOuterHashJoin, &gpopt::CTranslatorExprToDXL::PdxlnHashJoin},
			{COperator::EopPhysicalLeftSemiHashJoin, &gpopt::CTranslatorExprToDXL::PdxlnHashJoin},
			{COperator::EopPhysicalLeftAntiSemiHashJoin, &gpopt::CTranslatorExprToDXL::PdxlnHashJoin},
			{COperator::EopPhysicalLeftAntiSemiHashJoinNotIn, &gpopt::CTranslatorExprToDXL::PdxlnHashJoin},
			{COperator::EopPhysicalMotionGather,&gpopt::CTranslatorExprToDXL::PdxlnMotion},
			{COperator::EopPhysicalMotionBroadcast,&gpopt::CTranslatorExprToDXL::PdxlnMotion},
			{COperator::EopPhysicalMotionHashDistribute,&gpopt::CTranslatorExprToDXL::PdxlnMotion},
			{COperator::EopPhysicalMotionRoutedDistribute,&gpopt::CTranslatorExprToDXL::PdxlnMotion},
			{COperator::EopPhysicalMotionRandom, &gpopt::CTranslatorExprToDXL::PdxlnMotion},
			{COperator::EopPhysicalSpool, &gpopt::CTranslatorExprToDXL::PdxlnMaterialize},
			{COperator::EopPhysicalSequence, &gpopt::CTranslatorExprToDXL::PdxlnSequence},
			{COperator::EopPhysicalDynamicTableScan, &gpopt::CTranslatorExprToDXL::PdxlnDynamicTableScan},
			{COperator::EopPhysicalDynamicBitmapTableScan, &gpopt::CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan},
			{COperator::EopPhysicalDynamicIndexScan, &gpopt::CTranslatorExprToDXL::PdxlnDynamicIndexScan},
			{COperator::EopPhysicalPartitionSelector, &gpopt::CTranslatorExprToDXL::PdxlnPartitionSelector},
			{COperator::EopPhysicalPartitionSelectorDML, &gpopt::CTranslatorExprToDXL::PdxlnPartitionSelectorDML},
			{COperator::EopPhysicalConstTableGet, &gpopt::CTranslatorExprToDXL::PdxlnResultFromConstTableGet},
			{COperator::EopPhysicalTVF, &gpopt::CTranslatorExprToDXL::PdxlnTVF},
			{COperator::EopPhysicalSerialUnionAll, &gpopt::CTranslatorExprToDXL::PdxlnAppend},
			{COperator::EopPhysicalParallelUnionAll, &gpopt::CTranslatorExprToDXL::PdxlnAppend},
			{COperator::EopPhysicalDML, &gpopt::CTranslatorExprToDXL::PdxlnDML},
			{COperator::EopPhysicalSplit, &gpopt::CTranslatorExprToDXL::PdxlnSplit},
			{COperator::EopPhysicalRowTrigger, &gpopt::CTranslatorExprToDXL::PdxlnRowTrigger},
			{COperator::EopPhysicalAssert, &gpopt::CTranslatorExprToDXL::PdxlnAssert},
			{COperator::EopPhysicalCTEProducer, &gpopt::CTranslatorExprToDXL::PdxlnCTEProducer},
			{COperator::EopPhysicalCTEConsumer, &gpopt::CTranslatorExprToDXL::PdxlnCTEConsumer},
	};

	const ULONG ulTranslators = GPOS_ARRAY_SIZE(rgPhysicalTranslators);
	for (ULONG ul = 0; ul < ulTranslators; ul++)
	{
		SPhTranslatorMapping elem = rgPhysicalTranslators[ul];
		m_rgpfPhysicalTranslators[elem.eopid] = elem.pf;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTranslate
//
//	@doc:
//		Main driver
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnTranslate
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPmdname *pdrgpmdname
	)
{
	CAutoTimer at("\n[OPT]: Expr To DXL Translation Time", GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	GPOS_ASSERT(NULL == m_pdpplan);
	
	m_pdpplan = CDrvdPropPlan::Pdpplan(pexpr->PdpDerive());
	m_pdpplan->AddRef();

	DrgPds *pdrgpdsBaseTables = GPOS_NEW(m_memory_pool) DrgPds(m_memory_pool);
	ULONG ulNonGatherMotions = 0;
	BOOL fDML = false;
	CDXLNode *pdxln = CreateDXLNode(pexpr, pdrgpcr, pdrgpdsBaseTables, &ulNonGatherMotions, &fDML, true /*fRemap*/, true /*fRoot*/);

	if (fDML)
	{
		pdrgpdsBaseTables->Release();
		return pdxln;
	}

	CDXLNode *pdxlnPrL = (*pdxln)[0];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());

	const ULONG ulLen = pdrgpmdname->Size();
	GPOS_ASSERT(ulLen == pdrgpcr->Size());
	GPOS_ASSERT(ulLen == pdxlnPrL->Arity());
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		// desired output column name
		CMDName *mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, (*pdrgpmdname)[ul]->GetMDName());

		// get the old project element for the ColId
		CDXLNode *pdxlnPrElOld = (*pdxlnPrL)[ul];
		CDXLScalarProjElem *pdxlopPrElOld = CDXLScalarProjElem::Cast(pdxlnPrElOld->GetOperator());
		GPOS_ASSERT(1 == pdxlnPrElOld->Arity());
		CDXLNode *child_dxlnode = (*pdxlnPrElOld)[0];
		const ULONG col_id = pdxlopPrElOld->UlId();

		// create a new project element node with the col id and new column name
		// and add the scalar child
		CDXLNode *pdxlnPrElNew = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarProjElem(m_memory_pool, col_id, mdname));
		child_dxlnode->AddRef();
		pdxlnPrElNew->AddChild(child_dxlnode);

		// replace the project element
		pdxlnPrL->ReplaceChild(ul, pdxlnPrElNew);
	}

	

	if (0 == ulNonGatherMotions)
	{
		CDrvdPropRelational *pdprel =  CDrvdPropRelational::Pdprel(pexpr->Pdp(CDrvdProp::EptRelational));
		CTranslatorExprToDXLUtils::SetDirectDispatchInfo(m_memory_pool, m_pmda, pdxln, pdprel, pdrgpdsBaseTables);
	}
	
	pdrgpdsBaseTables->Release();
	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::CreateDXLNode
//
//	@doc:
//		Translates an optimizer physical expression tree into DXL.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::CreateDXLNode
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	BOOL fRemap,
	BOOL fRoot
	)
{
	GPOS_ASSERT(NULL != pexpr);
	ULONG ulOpId =  (ULONG) pexpr->Pop()->Eopid();
	if (COperator::EopPhysicalTableScan == ulOpId || COperator::EopPhysicalExternalScan == ulOpId)
	{
		CDXLNode *pdxln = PdxlnTblScan(pexpr, NULL /*pcrsOutput*/, pdrgpcr, pdrgpdsBaseTables, NULL /* pexprScalarCond */, NULL /* cost info */);
		CTranslatorExprToDXLUtils::SetStats(m_memory_pool, m_pmda, pdxln, pexpr->Pstats(), fRoot);
		
		return pdxln;
	}
	PfPdxlnPhysical pf = m_rgpfPhysicalTranslators[ulOpId];
	if (NULL == pf)
	{
		GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, pexpr->Pop()->SzId());
	}

	// add a result node on top to project out columns not needed any further,
	// for instance, if the grouping /order by /partition/ distribution columns
	// are no longer needed
	CDXLNode *pdxlnNew = NULL;

	CDXLNode *pdxln = (this->* pf)(pexpr, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

	if (!fRemap || EdxlopPhysicalDML == pdxln->GetOperator()->GetDXLOperator())
	{
		pdxlnNew = pdxln;
	}
	else
	{
		DrgPcr *pdrgpcrRequired = NULL;
		
		if (EdxlopPhysicalCTAS == pdxln->GetOperator()->GetDXLOperator())
		{
			pdrgpcr->AddRef();
			pdrgpcrRequired = pdrgpcr;
		}
		else
		{
			pdrgpcrRequired = pexpr->Prpp()->PcrsRequired()->Pdrgpcr(m_memory_pool);
		}
		pdxlnNew = PdxlnRemapOutputColumns(pexpr, pdxln, pdrgpcrRequired, pdrgpcr);
		pdrgpcrRequired->Release();
	}

	if (NULL == pdxlnNew->GetProperties()->Pdxlstatsderrel())
	{
		CTranslatorExprToDXLUtils::SetStats(m_memory_pool, m_pmda, pdxlnNew, pexpr->Pstats(), fRoot);
	}
	
	return pdxlnNew;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScalar
//
//	@doc:
//		Translates an optimizer scalar expression tree into DXL. Any column
//		refs that are members of the input colrefset are replaced by the input
//		subplan node
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScalar
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);

	ULONG ulOpId =  (ULONG) pexpr->Pop()->Eopid();
	PfPdxlnScalar pf = m_rgpfScalarTranslators[ulOpId];

	if (NULL == pf)
	{
		GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, pexpr->Pop()->SzId());
	}
	
	return (this->* pf)(pexpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTblScan
//
//	@doc:
//		Create a DXL table scan node from an optimizer table scan node
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnTblScan
	(
	CExpression *pexprTblScan,
	CColRefSet *pcrsOutput,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	CExpression *pexprScalar,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexprTblScan);

	CPhysicalTableScan *popTblScan = CPhysicalTableScan::PopConvert(pexprTblScan->Pop());
	DrgPcr *pdrgpcrOutput = popTblScan->PdrgpcrOutput();
	
	// translate table descriptor
	CDXLTableDescr *table_descr = MakeDXLTableDescr(popTblScan->Ptabdesc(), pdrgpcrOutput);

	// construct plan costs, if there are not passed as a parameter
	if (NULL == dxl_properties)
	{
		dxl_properties = GetProperties(pexprTblScan);
	}

	// construct scan operator
	CDXLPhysicalTableScan *pdxlopTS = NULL;
	COperator::EOperatorId eopid = pexprTblScan->Pop()->Eopid();
	if (COperator::EopPhysicalTableScan == eopid)
	{
		pdxlopTS = GPOS_NEW(m_memory_pool) CDXLPhysicalTableScan(m_memory_pool, table_descr);
	}
	else
	{
		GPOS_ASSERT(COperator::EopPhysicalExternalScan == eopid);
		pdxlopTS = GPOS_NEW(m_memory_pool) CDXLPhysicalExternalScan(m_memory_pool, table_descr);
	}
	
	CDXLNode *pdxlnTblScan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopTS);
	pdxlnTblScan->SetProperties(dxl_properties);
	
	// construct projection list
	GPOS_ASSERT(NULL != pexprTblScan->Prpp());

	// if the output columns are passed from above then use them
	if (NULL == pcrsOutput)
	{
	  pcrsOutput = pexprTblScan->Prpp()->PcrsRequired();
	}
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcr);

	CDXLNode *pdxlnCond = NULL;
	if (NULL != pexprScalar)
	{
	  pdxlnCond = PdxlnScalar(pexprScalar);
	}

	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);
	
	// add children in the right order
	pdxlnTblScan->AddChild(pdxlnPrL); 		// project list
	pdxlnTblScan->AddChild(filter_dxlnode);	// filter
	
#ifdef GPOS_DEBUG
	pdxlnTblScan->GetOperator()->AssertValid(pdxlnTblScan, false /* validate_children */);
#endif
	
	CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprTblScan->Pdp(CDrvdProp::EptPlan))->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(pds);
	return pdxlnTblScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScan
//
//	@doc:
//		Create a DXL index scan node from an optimizer index scan node based
//		on passed properties
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnIndexScan
	(
	CExpression *pexprIndexScan,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	GPOS_ASSERT(NULL != pexprIndexScan);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprIndexScan);

	CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprIndexScan->Pdp(CDrvdProp::EptPlan))->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(pds);
	return PdxlnIndexScan(pexprIndexScan, pdrgpcr, dxl_properties, pexprIndexScan->Prpp());
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScan
//
//	@doc:
//		Create a DXL index scan node from an optimizer index scan node
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnIndexScan
	(
	CExpression *pexprIndexScan,
	DrgPcr *pdrgpcr,
	CDXLPhysicalProperties *dxl_properties,
	CReqdPropPlan *prpp
	)
{
	GPOS_ASSERT(NULL != pexprIndexScan);
	GPOS_ASSERT(NULL != dxl_properties);
	GPOS_ASSERT(NULL != prpp);

	CPhysicalIndexScan *popIs = CPhysicalIndexScan::PopConvert(pexprIndexScan->Pop());

	DrgPcr *pdrgpcrOutput = popIs->PdrgpcrOutput();

	// translate table descriptor
	CDXLTableDescr *table_descr = MakeDXLTableDescr(popIs->Ptabdesc(), pdrgpcrOutput);

	// create index descriptor
	CIndexDescriptor *pindexdesc = popIs->Pindexdesc();
	CMDName *pmdnameIndex = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pindexdesc->Name().Pstr());
	IMDId *pmdidIndex = pindexdesc->MDId();
	pmdidIndex->AddRef();
	CDXLIndexDescr *index_descr_dxl = GPOS_NEW(m_memory_pool) CDXLIndexDescr(m_memory_pool, pmdidIndex, pmdnameIndex);

	// TODO: vrgahavan; we assume that the index are always forward access.
	// create the physical index scan operator
	CDXLPhysicalIndexScan *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalIndexScan(m_memory_pool, table_descr, index_descr_dxl, EdxlisdForward);
	CDXLNode *pdxlnIndexScan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// set properties
	pdxlnIndexScan->SetProperties(dxl_properties);

	// translate project list
	CColRefSet *pcrsOutput = prpp->PcrsRequired();
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcr);

	// translate index predicates
	CExpression *pexprCond = (*pexprIndexScan)[0];
	CDXLNode *pdxlnIndexCondList = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarIndexCondList(m_memory_pool));

	DrgPexpr *pdrgpexprConds = CPredicateUtils::PdrgpexprConjuncts(m_memory_pool, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CExpression *pexprIndexCond = (*pdrgpexprConds)[ul];
		CDXLNode *pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();

	CDXLNode *pdxlnResidualCond = NULL;
	if (2 == pexprIndexScan->Arity())
	{
		// translate residual predicates into the filter node
		CExpression *pexprResidualCond = (*pexprIndexScan)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnResidualCond);

	pdxlnIndexScan->AddChild(pdxlnPrL);
	pdxlnIndexScan->AddChild(filter_dxlnode);
	pdxlnIndexScan->AddChild(pdxlnIndexCondList);

#ifdef GPOS_DEBUG
	pdxlnIndexScan->GetOperator()->AssertValid(pdxlnIndexScan, false /* validate_children */);
#endif


	return pdxlnIndexScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapIndexProbe
//
//	@doc:
//		Create a DXL scalar bitmap index probe from an optimizer
//		scalar bitmap index probe operator.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnBitmapIndexProbe
	(
	CExpression *pexprBitmapIndexProbe
	)
{
	GPOS_ASSERT(NULL != pexprBitmapIndexProbe);
	CScalarBitmapIndexProbe *pop = CScalarBitmapIndexProbe::PopConvert(pexprBitmapIndexProbe->Pop());

	// create index descriptor
	CIndexDescriptor *pindexdesc = pop->Pindexdesc();
	CMDName *pmdnameIndex = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pindexdesc->Name().Pstr());
	IMDId *pmdidIndex = pindexdesc->MDId();
	pmdidIndex->AddRef();

	CDXLIndexDescr *index_descr_dxl = GPOS_NEW(m_memory_pool) CDXLIndexDescr(m_memory_pool, pmdidIndex, pmdnameIndex);
	CDXLScalarBitmapIndexProbe *dxl_op = GPOS_NEW(m_memory_pool) CDXLScalarBitmapIndexProbe(m_memory_pool, index_descr_dxl);
	CDXLNode *pdxlnBitmapIndexProbe = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// translate index predicates
	CExpression *pexprCond = (*pexprBitmapIndexProbe)[0];
	CDXLNode *pdxlnIndexCondList = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarIndexCondList(m_memory_pool));
	DrgPexpr *pdrgpexprConds = CPredicateUtils::PdrgpexprConjuncts(m_memory_pool, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CExpression *pexprIndexCond = (*pdrgpexprConds)[ul];
		CDXLNode *pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();
	pdxlnBitmapIndexProbe->AddChild(pdxlnIndexCondList);

#ifdef GPOS_DEBUG
	pdxlnBitmapIndexProbe->GetOperator()->AssertValid(pdxlnBitmapIndexProbe, false /*validate_children*/);
#endif

	return pdxlnBitmapIndexProbe;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapBoolOp
//
//	@doc:
//		Create a DXL scalar bitmap boolean operator from an optimizer
//		scalar bitmap boolean operator operator.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnBitmapBoolOp
	(
	CExpression *pexprBitmapBoolOp
	)
{
	GPOS_ASSERT(NULL != pexprBitmapBoolOp);
	GPOS_ASSERT(2 == pexprBitmapBoolOp->Arity());
	
	CScalarBitmapBoolOp *popBitmapBoolOp = CScalarBitmapBoolOp::PopConvert(pexprBitmapBoolOp->Pop());
	CExpression *pexprLeft = (*pexprBitmapBoolOp)[0];
	CExpression *pexprRight = (*pexprBitmapBoolOp)[1];
	
	CDXLNode *pdxlnLeft = PdxlnScalar(pexprLeft);
	CDXLNode *pdxlnRight = PdxlnScalar(pexprRight);
	
	IMDId *mdid_type = popBitmapBoolOp->MDIdType();
	mdid_type->AddRef();
	
	CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp edxlbitmapop = CDXLScalarBitmapBoolOp::EdxlbitmapAnd;
	
	if (CScalarBitmapBoolOp::EbitmapboolOr == popBitmapBoolOp->Ebitmapboolop())
	{
		edxlbitmapop = CDXLScalarBitmapBoolOp::EdxlbitmapOr;
	}
	
	return GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarBitmapBoolOp(m_memory_pool, mdid_type, edxlbitmapop),
						pdxlnLeft,
						pdxlnRight
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapTableScan
//
//	@doc:
//		Create a DXL physical bitmap table scan from an optimizer
//		physical bitmap table scan operator.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnBitmapTableScan
	(
	CExpression *pexprBitmapTableScan,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	return PdxlnBitmapTableScan
			(
			pexprBitmapTableScan,
			NULL,  // pcrsOutput
			pdrgpcr,
			pdrgpdsBaseTables, 
			NULL, // pexprScalar
			NULL // dxl_properties
			);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::AddBitmapFilterColumns
//
//	@doc:
//		Add used columns in the bitmap recheck and the remaining scalar filter
//		condition to the required output column
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::AddBitmapFilterColumns
	(
	IMemoryPool *memory_pool,
	CPhysicalScan *pop,
	CExpression *pexprRecheckCond,
	CExpression *pexprScalar,
	CColRefSet *pcrsReqdOutput // append the required column reference
	)
{
	GPOS_ASSERT(NULL != pop);
	GPOS_ASSERT(COperator::EopPhysicalDynamicBitmapTableScan == pop->Eopid() ||
				COperator::EopPhysicalBitmapTableScan == pop->Eopid());
	GPOS_ASSERT(NULL != pcrsReqdOutput);

	// compute what additional columns are required in the output of the (Dynamic) Bitmap Table Scan
	CColRefSet *pcrsAdditional =  GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	if (NULL != pexprRecheckCond)
	{
		// add the columns used in the recheck condition
		pcrsAdditional->Include(CDrvdPropScalar::Pdpscalar(pexprRecheckCond->PdpDerive())->PcrsUsed());
	}

	if (NULL != pexprScalar)
	{
		// add the columns used in the filter condition
		pcrsAdditional->Include(CDrvdPropScalar::Pdpscalar(pexprScalar->PdpDerive())->PcrsUsed());
	}

	CColRefSet *pcrsBitmap =  GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	pcrsBitmap->Include(pop->PdrgpcrOutput());

	// only keep the columns that are in the table associated with the bitmap
	pcrsAdditional->Intersection(pcrsBitmap);

	if (0 < pcrsAdditional->Size())
	{
		pcrsReqdOutput->Include(pcrsAdditional);
	}

	// clean up
	pcrsAdditional->Release();
	pcrsBitmap->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBitmapTableScan
//
//	@doc:
//		Create a DXL physical bitmap table scan from an optimizer
//		physical bitmap table scan operator.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnBitmapTableScan
	(
	CExpression *pexprBitmapTableScan,
	CColRefSet *pcrsOutput,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	CExpression *pexprScalar,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexprBitmapTableScan);
	CPhysicalBitmapTableScan *pop = CPhysicalBitmapTableScan::PopConvert(pexprBitmapTableScan->Pop());

	// translate table descriptor
	CDXLTableDescr *table_descr = MakeDXLTableDescr(pop->Ptabdesc(), pop->PdrgpcrOutput());

	CDXLPhysicalBitmapTableScan *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalBitmapTableScan(m_memory_pool, table_descr);
	CDXLNode *pdxlnBitmapTableScan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// set properties
	// construct plan costs, if there are not passed as a parameter
	if (NULL == dxl_properties)
	{
		dxl_properties = GetProperties(pexprBitmapTableScan);
	}
	pdxlnBitmapTableScan->SetProperties(dxl_properties);

	// build projection list
	if (NULL == pcrsOutput)
	{
		pcrsOutput = pexprBitmapTableScan->Prpp()->PcrsRequired();
	}

	// translate scalar predicate into DXL filter only if it is not redundant
	CExpression *pexprRecheckCond = (*pexprBitmapTableScan)[0];
	CDXLNode *pdxlnCond = NULL;
	if (NULL != pexprScalar &&
		!CUtils::FScalarConstTrue(pexprScalar) &&
		!pexprScalar->FMatch(pexprRecheckCond))
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}

	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);

	CDXLNode *pdxlnRecheckCond = PdxlnScalar(pexprRecheckCond);
	CDXLNode *pdxlnRecheckCondFilter =
			GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarRecheckCondFilter(m_memory_pool), pdxlnRecheckCond
						);

	AddBitmapFilterColumns(m_memory_pool, pop, pexprRecheckCond, pexprScalar, pcrsOutput);

	CDXLNode *proj_list_dxlnode = PdxlnProjList(pcrsOutput, pdrgpcr);

	// translate bitmap access path
	CDXLNode *pdxlnBitmapIndexPath = PdxlnScalar((*pexprBitmapTableScan)[1]);

	pdxlnBitmapTableScan->AddChild(proj_list_dxlnode);
	pdxlnBitmapTableScan->AddChild(filter_dxlnode);
	pdxlnBitmapTableScan->AddChild(pdxlnRecheckCondFilter);
	pdxlnBitmapTableScan->AddChild(pdxlnBitmapIndexPath);
#ifdef GPOS_DEBUG
	pdxlnBitmapTableScan->GetOperator()->AssertValid(pdxlnBitmapTableScan, false /*validate_children*/);
#endif
	
	CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprBitmapTableScan->Pdp(CDrvdProp::EptPlan))->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(pds);

	return pdxlnBitmapTableScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicTableScan
//
//	@doc:
//		Create a DXL dynamic table scan node from an optimizer 
//		dynamic table scan node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDynamicTableScan
	(
	CExpression *pexprDTS,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	CExpression *pexprScalarCond = NULL;
	CDXLPhysicalProperties *dxl_properties = NULL;
	return PdxlnDynamicTableScan(pexprDTS, pdrgpcr, pdrgpdsBaseTables, pexprScalarCond, dxl_properties);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicTableScan
//
//	@doc:
//		Create a DXL dynamic table scan node from an optimizer 
//		dynamic table scan node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDynamicTableScan
	(
	CExpression *pexprDTS,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	CExpression *pexprScalarCond,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexprDTS);
	GPOS_ASSERT_IFF(NULL != pexprScalarCond, NULL != dxl_properties);
	
	CPhysicalDynamicTableScan *popDTS = CPhysicalDynamicTableScan::PopConvert(pexprDTS->Pop());	
	DrgPcr *pdrgpcrOutput = popDTS->PdrgpcrOutput();
	
	// translate table descriptor
	CDXLTableDescr *table_descr = MakeDXLTableDescr(popDTS->Ptabdesc(), pdrgpcrOutput);

	// construct plan costs
	CDXLPhysicalProperties *pdxlpropDTS = GetProperties(pexprDTS);
	
	if (NULL != dxl_properties)
	{
		CWStringDynamic *rows_out_str = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool, dxl_properties->MakeDXLOperatorCost()->GetRowsOutStr()->GetBuffer());
		CWStringDynamic *pstrCost = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool, dxl_properties->MakeDXLOperatorCost()->GetTotalCostStr()->GetBuffer());

		pdxlpropDTS->MakeDXLOperatorCost()->SetRows(rows_out_str);
		pdxlpropDTS->MakeDXLOperatorCost()->SetCost(pstrCost);
		dxl_properties->Release();
	}

	// construct dynamic table scan operator
	CDXLPhysicalDynamicTableScan *pdxlopDTS = 
			GPOS_NEW(m_memory_pool) CDXLPhysicalDynamicTableScan
						(
						m_memory_pool, 
						table_descr, 
						popDTS->UlSecondaryScanId(),
						popDTS->UlScanId()
						);

	CDXLNode *pdxlnDTS = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopDTS);
	pdxlnDTS->SetProperties(pdxlpropDTS);
	
	CDXLNode *pdxlnCond = NULL;
	
	if (NULL != pexprScalarCond)
	{
		pdxlnCond = PdxlnScalar(pexprScalarCond);
	}
	
	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);
	
	// construct projection list
	GPOS_ASSERT(NULL != pexprDTS->Prpp());
	
	CColRefSet *pcrsOutput = pexprDTS->Prpp()->PcrsRequired();
	pdxlnDTS->AddChild(PdxlnProjList(pcrsOutput, pdrgpcr));
	pdxlnDTS->AddChild(filter_dxlnode);
	
#ifdef GPOS_DEBUG
	pdxlnDTS->GetOperator()->AssertValid(pdxlnDTS, false /* validate_children */);
#endif
	
	CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprDTS->Pdp(CDrvdProp::EptPlan))->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(pds);
	
	return pdxlnDTS;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan
//
//	@doc:
//		Create a DXL dynamic bitmap table scan node from an optimizer
//		dynamic bitmap table scan node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan
	(
	CExpression *pexprScan,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	CExpression *pexprScalar = NULL;
	CDXLPhysicalProperties *dxl_properties = NULL;
	return PdxlnDynamicBitmapTableScan
			(
			pexprScan,
			pdrgpcr,
			pdrgpdsBaseTables,
			pexprScalar,
			dxl_properties
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan
//
//	@doc:
//		Create a DXL dynamic bitmap table scan node from an optimizer
//		dynamic bitmap table scan node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDynamicBitmapTableScan
	(
	CExpression *pexprScan,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	CExpression *pexprScalar,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexprScan);

	CPhysicalDynamicBitmapTableScan *pop = CPhysicalDynamicBitmapTableScan::PopConvert(pexprScan->Pop());
	DrgPcr *pdrgpcrOutput = pop->PdrgpcrOutput();

	CDXLTableDescr *table_descr = MakeDXLTableDescr(pop->Ptabdesc(), pdrgpcrOutput);
	CDXLPhysicalDynamicBitmapTableScan *pdxlopScan =
			GPOS_NEW(m_memory_pool) CDXLPhysicalDynamicBitmapTableScan
						(
						m_memory_pool,
						table_descr,
						pop->UlSecondaryScanId(),
						pop->UlScanId()
						);

	CDXLNode *pdxlnScan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopScan);

	// construct plan costs
	if (NULL == dxl_properties)
	{
		dxl_properties = GetProperties(pexprScan);
	}
	pdxlnScan->SetProperties(dxl_properties);

	// translate predicates into DXL filter
	CDXLNode *pdxlnCond = NULL;
	if (NULL != pexprScalar)
	{
		pdxlnCond = PdxlnScalar(pexprScalar);
	}
	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);

	CExpression *pexprRecheckCond = (*pexprScan)[0];
	CDXLNode *pdxlnRecheckCond = PdxlnScalar(pexprRecheckCond);
	CDXLNode *pdxlnRecheckCondFilter =
			GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarRecheckCondFilter(m_memory_pool));
	pdxlnRecheckCondFilter->AddChild(pdxlnRecheckCond);

	// translate bitmap access path
	CDXLNode *pdxlnBitmapAccessPath = PdxlnScalar((*pexprScan)[1]);

	// build projection list
	CColRefSet *pcrsOutput = pexprScan->Prpp()->PcrsRequired();
	AddBitmapFilterColumns(m_memory_pool, pop, pexprRecheckCond, pexprScalar, pcrsOutput);
	CDXLNode *proj_list_dxlnode = PdxlnProjList(pcrsOutput, pdrgpcr);

	pdxlnScan->AddChild(proj_list_dxlnode);
	pdxlnScan->AddChild(filter_dxlnode);
	pdxlnScan->AddChild(pdxlnRecheckCondFilter);
	pdxlnScan->AddChild(pdxlnBitmapAccessPath);

#ifdef GPOS_DEBUG
	pdxlnScan->GetOperator()->AssertValid(pdxlnScan, false /* validate_children */);
#endif

	CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprScan->Pdp(CDrvdProp::EptPlan))->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(pds);
	
	return pdxlnScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicIndexScan
//
//	@doc:
//		Create a DXL dynamic index scan node from an optimizer 
//		dynamic index scan node based on passed properties
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDynamicIndexScan
	(
	CExpression *pexprDIS,
	DrgPcr *pdrgpcr,
	CDXLPhysicalProperties *dxl_properties,
	CReqdPropPlan *prpp
	)
{
	GPOS_ASSERT(NULL != pexprDIS);
	GPOS_ASSERT(NULL != dxl_properties);
	GPOS_ASSERT(NULL != prpp);

	CPhysicalDynamicIndexScan *popDIS = CPhysicalDynamicIndexScan::PopConvert(pexprDIS->Pop());	
	DrgPcr *pdrgpcrOutput = popDIS->PdrgpcrOutput();
	
	// translate table descriptor
	CDXLTableDescr *table_descr = MakeDXLTableDescr(popDIS->Ptabdesc(), pdrgpcrOutput);

	// create index descriptor
	CIndexDescriptor *pindexdesc = popDIS->Pindexdesc();
	CMDName *pmdnameIndex = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pindexdesc->Name().Pstr());
	IMDId *pmdidIndex = pindexdesc->MDId();
	pmdidIndex->AddRef();
	CDXLIndexDescr *index_descr_dxl = GPOS_NEW(m_memory_pool) CDXLIndexDescr(m_memory_pool, pmdidIndex, pmdnameIndex);

	// TODO: vrgahavan; we assume that the index are always forward access.
	// create the physical index scan operator
	CDXLNode *pdxlnDIS = GPOS_NEW(m_memory_pool) CDXLNode
									(
									m_memory_pool, 
									GPOS_NEW(m_memory_pool) CDXLPhysicalDynamicIndexScan
													(
													m_memory_pool,
													table_descr,
													popDIS->UlSecondaryScanId(),
													popDIS->UlScanId(),
													index_descr_dxl,
													EdxlisdForward
													)
									);
	
	// set plan costs
	pdxlnDIS->SetProperties(dxl_properties);
	
	// construct projection list
	CColRefSet *pcrsOutput = prpp->PcrsRequired();
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcr);
	
	// translate index predicates
	CExpression *pexprCond = (*pexprDIS)[0];
	CDXLNode *pdxlnIndexCondList = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarIndexCondList(m_memory_pool));

	DrgPexpr *pdrgpexprConds = CPredicateUtils::PdrgpexprConjuncts(m_memory_pool, pexprCond);
	const ULONG length = pdrgpexprConds->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CExpression *pexprIndexCond = (*pdrgpexprConds)[ul];
		CDXLNode *pdxlnIndexCond = PdxlnScalar(pexprIndexCond);
		pdxlnIndexCondList->AddChild(pdxlnIndexCond);
	}
	pdrgpexprConds->Release();

	CDXLNode *pdxlnResidualCond = NULL;
	if (2 == pexprDIS->Arity())
	{
		// translate residual predicates into the filter node
		CExpression *pexprResidualCond = (*pexprDIS)[1];
		if (COperator::EopScalarConst != pexprResidualCond->Pop()->Eopid())
		{
			pdxlnResidualCond = PdxlnScalar(pexprResidualCond);
		}
	}

	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnResidualCond);

	pdxlnDIS->AddChild(pdxlnPrL);
	pdxlnDIS->AddChild(filter_dxlnode);
	pdxlnDIS->AddChild(pdxlnIndexCondList);
	
#ifdef GPOS_DEBUG
	pdxlnDIS->GetOperator()->AssertValid(pdxlnDIS, false /* validate_children */);
#endif
		
	return pdxlnDIS;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDynamicIndexScan
//
//	@doc:
//		Create a DXL dynamic index scan node from an optimizer
//		dynamic index scan node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDynamicIndexScan
	(
	CExpression *pexprDIS,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	GPOS_ASSERT(NULL != pexprDIS);

	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprDIS);

	CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprDIS->Pdp(CDrvdProp::EptPlan))->Pds();
	pds->AddRef();
	pdrgpdsBaseTables->Append(pds);
	return PdxlnDynamicIndexScan(pexprDIS, pdrgpcr, dxl_properties, pexprDIS->Prpp());
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node over a relational expression with a DXL
//		scalar condition.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResult
	(
	CExpression *pexprRelational,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CDXLNode *pdxlnScalar
	)
{
	// extract physical properties from filter
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprRelational);

	return PdxlnResult(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, pdxlnScalar, dxl_properties);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node over a relational expression with a DXL
//		scalar condition using the passed DXL properties
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResult
	(
	CExpression *pexprRelational,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CDXLNode *pdxlnScalar,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexprRelational);

	// translate relational child expression
	CDXLNode *pdxlnRelationalChild = CreateDXLNode(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot */);
	GPOS_ASSERT(NULL != pexprRelational->Prpp());
	CColRefSet *pcrsOutput = pexprRelational->Prpp()->PcrsRequired();

	return PdxlnAddScalarFilterOnRelationalChild
			(
			pdxlnRelationalChild,
			pdxlnScalar,
			dxl_properties,
			pcrsOutput,
			pdrgpcr
			);
}

CDXLNode *
CTranslatorExprToDXL::PdxlnAddScalarFilterOnRelationalChild
	(
	CDXLNode *pdxlnRelationalChild,
	CDXLNode *pdxlnScalarChild,
	CDXLPhysicalProperties *dxl_properties,
	CColRefSet *pcrsOutput,
	DrgPcr *pdrgpcrOrder
	)
{
	GPOS_ASSERT(NULL != dxl_properties);
	// for a true condition, just translate the child
	if (CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda, pdxlnScalarChild))
	{
		pdxlnScalarChild->Release();
		dxl_properties->Release();
		return pdxlnRelationalChild;
	}
	// create a result node over outer child
	else
	{
		// wrap condition in a DXL filter node
		CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnScalarChild);

		CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrOrder);

		// create an empty one-time filter
		CDXLNode *pdxlnOneTimeFilter = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool));

		return CTranslatorExprToDXLUtils::PdxlnResult
				(
				m_memory_pool,
				dxl_properties,
				pdxlnPrL,
				filter_dxlnode,
				pdxlnOneTimeFilter,
				pdxlnRelationalChild
				);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node from an optimizer filter node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResult
	(
	CExpression *pexprFilter,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprFilter);

	CDXLNode *pdxlnode = PdxlnResult(pexprFilter, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, dxl_properties);
	dxl_properties->Release();

	return pdxlnode;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnIndexScanWithInlinedCondition
//
//	@doc:
//		Create a (dynamic) index scan node after inlining the given
//		scalar condition, if needed
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnIndexScanWithInlinedCondition
	(
	CExpression *pexprIndexScan,
	CExpression *pexprScalarCond,
	CDXLPhysicalProperties *dxl_properties,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables
	)
{
	GPOS_ASSERT(NULL != pexprIndexScan);
	GPOS_ASSERT(NULL != pexprScalarCond);
	GPOS_ASSERT(pexprScalarCond->Pop()->FScalar());

	COperator::EOperatorId eopid = pexprIndexScan->Pop()->Eopid();
	GPOS_ASSERT(COperator::EopPhysicalIndexScan == eopid ||
			COperator::EopPhysicalDynamicIndexScan == eopid);

	// inline scalar condition in index scan, if it is not the same as index lookup condition
	CExpression *pexprIndexLookupCond = (*pexprIndexScan)[0];
	CDXLNode *pdxlnIndexScan = NULL;
	if (!CUtils::FScalarConstTrue(pexprScalarCond) && !pexprScalarCond->FMatch(pexprIndexLookupCond))
	{
		// combine scalar condition with existing index conditions, if any
		pexprScalarCond->AddRef();
		CExpression *pexprNewScalarCond = pexprScalarCond;
		if (2 == pexprIndexScan->Arity())
		{
			pexprNewScalarCond->Release();
			pexprNewScalarCond = CPredicateUtils::PexprConjunction(m_memory_pool, (*pexprIndexScan)[1], pexprScalarCond);
		}
		pexprIndexLookupCond->AddRef();
		pexprIndexScan->Pop()->AddRef();
		CExpression *pexprNewIndexScan = GPOS_NEW(m_memory_pool) CExpression(m_memory_pool, pexprIndexScan->Pop(), pexprIndexLookupCond, pexprNewScalarCond);
		if (COperator::EopPhysicalIndexScan == eopid)
		{
			pdxlnIndexScan = PdxlnIndexScan(pexprNewIndexScan, pdrgpcr, dxl_properties, pexprIndexScan->Prpp());
		}
		else
		{
			pdxlnIndexScan = PdxlnDynamicIndexScan(pexprNewIndexScan, pdrgpcr, dxl_properties, pexprIndexScan->Prpp());
		}
		pexprNewIndexScan->Release();

		CDistributionSpec *pds = CDrvdPropPlan::Pdpplan(pexprIndexScan->Pdp(CDrvdProp::EptPlan))->Pds();
		pds->AddRef();
		pdrgpdsBaseTables->Append(pds);
		
		return pdxlnIndexScan;
	}

	// index scan does not need the properties of the filter, as it does not
	// need to further inline the scalar condition
	dxl_properties->Release();
	ULONG ulNonGatherMotions = 0;
	BOOL fDML = false;
	if (COperator::EopPhysicalIndexScan == eopid)
	{
		return PdxlnIndexScan(pexprIndexScan, pdrgpcr, pdrgpdsBaseTables, &ulNonGatherMotions, &fDML);
	}

	return PdxlnDynamicIndexScan(pexprIndexScan, pdrgpcr, pdrgpdsBaseTables, &ulNonGatherMotions, &fDML);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResult
//
//	@doc:
//		Create a DXL result node from an optimizer filter node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResult
	(
	CExpression *pexprFilter,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexprFilter);
	GPOS_ASSERT(NULL != dxl_properties);

	// extract components
	CExpression *pexprRelational = (*pexprFilter)[0];
	CExpression *pexprScalar = (*pexprFilter)[1];

	// if the filter predicate is a constant TRUE, skip to translating relational child
	if (CUtils::FScalarConstTrue(pexprScalar))
	{
		return CreateDXLNode(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, false /* fRoot */);
	}

	COperator::EOperatorId eopidRelational = pexprRelational->Pop()->Eopid();
	CColRefSet *pcrsOutput = pexprFilter->Prpp()->PcrsRequired();

	switch (eopidRelational)
	{
		case COperator::EopPhysicalTableScan:
		case COperator::EopPhysicalExternalScan:
		{
			// if there is a structure of the form
			// 		filter->tablescan, or filter->CTG then
			// push the scalar filter expression to the tablescan/CTG respectively
			dxl_properties->AddRef();

			// translate the table scan with the filter condition
			return PdxlnTblScan
					(
					pexprRelational,
					pcrsOutput,
					NULL /* pdrgpcr */,
					pdrgpdsBaseTables, 
					pexprScalar,
					dxl_properties /* cost info */
					);
		}
		case COperator::EopPhysicalBitmapTableScan:
		{
			dxl_properties->AddRef();

			return PdxlnBitmapTableScan
					(
					pexprRelational,
					pcrsOutput,
					NULL /*pdrgpcr*/,
					pdrgpdsBaseTables,
					pexprScalar,
					dxl_properties
					);
		}
		case COperator::EopPhysicalDynamicTableScan:
		{
			dxl_properties->AddRef();

			// inline condition in the Dynamic Table Scan
			return PdxlnDynamicTableScan(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pexprScalar, dxl_properties);
		}
		case COperator::EopPhysicalIndexScan:
		case COperator::EopPhysicalDynamicIndexScan:
		{
			dxl_properties->AddRef();
			return PdxlnIndexScanWithInlinedCondition
						(
						pexprRelational, 
						pexprScalar,
						dxl_properties,
						pdrgpcr,
						pdrgpdsBaseTables
						);
		}
		case COperator::EopPhysicalDynamicBitmapTableScan:
		{
			dxl_properties->AddRef();

			return PdxlnDynamicBitmapTableScan(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pexprScalar, dxl_properties);
		}
		default:
		{
			return PdxlnResultFromFilter(pexprFilter, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
		}
	}
	
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelectorWithInlinedCondition
//
//	@doc:
//		Translate a partition selector into DXL while inlining the given
//		condition in the child
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelectorWithInlinedCondition
	(
	CExpression *pexprFilter,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprFilter);
	GPOS_ASSERT(COperator::EopPhysicalFilter == pexprFilter->Pop()->Eopid());
	GPOS_ASSERT(COperator::EopPhysicalPartitionSelector == (*pexprFilter)[0]->Pop()->Eopid());

	CExpression *pexprRelational = (*pexprFilter)[0];
	CExpression *pexprScalar = (*pexprFilter)[1];
	CExpression *pexprChild = (*pexprRelational)[0];
	COperator::EOperatorId eopid = pexprChild->Pop()->Eopid();
	BOOL fTableScanChild = (COperator::EopPhysicalDynamicTableScan == eopid);
	BOOL fIndexChild = (COperator::EopPhysicalDynamicIndexScan == eopid || COperator::EopPhysicalDynamicBitmapTableScan == eopid);
	GPOS_ASSERT(fTableScanChild || fIndexChild);

	// inline condition in child operator if the following conditions are met:
	BOOL fInlineCondition =
		NULL != pexprScalar &&	// condition is not NULL
		!CUtils::FScalarConstTrue(pexprScalar) &&	// condition is not const True
		(
		fTableScanChild || 	// child operator is TableScan
		(fIndexChild && !pexprScalar->FMatch((*pexprChild)[0]))	// OR, child operator is IndexScan and condition does not match index condition
		);

	CExpression *pexprCond = NULL;
	CDXLPhysicalProperties *dxl_properties = NULL;
	if (fInlineCondition)
	{
		pexprCond = pexprScalar;
		dxl_properties = GetProperties(pexprFilter);
	}

	return PdxlnPartitionSelector(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, pexprCond, dxl_properties);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromFilter
//
//	@doc:
//		Create a DXL result node from an optimizer filter node.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResultFromFilter
	(
	CExpression *pexprFilter,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprFilter);

	// extract components
	CExpression *pexprRelational = (*pexprFilter)[0];
	CExpression *pexprScalar = (*pexprFilter)[1];
	CColRefSet *pcrsOutput = pexprFilter->Prpp()->PcrsRequired();

	if (COperator::EopPhysicalPartitionSelector == pexprRelational->Pop()->Eopid())
	{
		COperator::EOperatorId eopid = (*pexprRelational)[0]->Pop()->Eopid();
		if (COperator::EopPhysicalDynamicIndexScan == eopid ||
			COperator::EopPhysicalDynamicBitmapTableScan == eopid ||
			COperator::EopPhysicalDynamicTableScan == eopid)
		{
			return PdxlnPartitionSelectorWithInlinedCondition(pexprFilter, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
		}
	}

	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprFilter);

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprRelational, NULL /* pdrgpcr */, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate scalar expression
	CDXLNode *pdxlnCond = PdxlnScalar(pexprScalar);

	// wrap condition in a DXL filter node
	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);

	GPOS_ASSERT(NULL != pexprFilter->Prpp());

	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcr);

	// create an empty one-time filter
	CDXLNode *pdxlnOneTimeFilter = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool));

	return CTranslatorExprToDXLUtils::PdxlnResult
											(
											m_memory_pool,
											dxl_properties,
											pdxlnPrL,
											filter_dxlnode,
											pdxlnOneTimeFilter,
											child_dxlnode
											);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssert
//
//	@doc:
//		Translate a physical assert expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAssert
	(
	CExpression *pexprAssert,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprAssert);

	// extract components
	CExpression *pexprRelational = (*pexprAssert)[0];
	CExpression *pexprScalar = (*pexprAssert)[1];
	CPhysicalAssert *popAssert = CPhysicalAssert::PopConvert(pexprAssert->Pop());
	
	// extract physical properties from assert
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprAssert);

	CColRefSet *pcrsOutput = pexprAssert->Prpp()->PcrsRequired();

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprRelational, NULL /* pdrgpcr */, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate scalar expression
	CDXLNode *pdxlnAssertPredicate = PdxlnScalar(pexprScalar);

	GPOS_ASSERT(NULL != pexprAssert->Prpp());

	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcr);

	const CHAR *sql_state = popAssert->Pexc()->GetSQLState();
	CDXLPhysicalAssert *pdxlopAssert = GPOS_NEW(m_memory_pool) CDXLPhysicalAssert(m_memory_pool, sql_state);
	CDXLNode *pdxlnAssert = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopAssert, pdxlnPrL, pdxlnAssertPredicate, child_dxlnode);
	
	pdxlnAssert->SetProperties(dxl_properties);
	
	return pdxlnAssert;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCTEProducer
//
//	@doc:
//		Translate a physical cte producer expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnCTEProducer
	(
	CExpression *pexprCTEProducer,
	DrgPcr * , //pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprCTEProducer);

	// extract components
	CExpression *pexprRelational = (*pexprCTEProducer)[0];
	CPhysicalCTEProducer *popCTEProducer = CPhysicalCTEProducer::PopConvert(pexprCTEProducer->Pop());

	// extract physical properties from cte producer
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprCTEProducer);

	// extract the CTE id and the array of colids
	const ULONG ulCTEId = popCTEProducer->UlCTEId();
	ULongPtrArray *pdrgpulColIds = CUtils::Pdrgpul(m_memory_pool, popCTEProducer->Pdrgpcr());

	GPOS_ASSERT(NULL != pexprCTEProducer->Prpp());
	DrgPcr *pdrgpcrRequired = popCTEProducer->Pdrgpcr();
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pdrgpcrRequired);

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprRelational, pdrgpcrRequired, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot */);

	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrRequired);
	pcrsOutput->Release();

	CDXLNode *pdxlnCTEProducer = GPOS_NEW(m_memory_pool) CDXLNode
										(
										m_memory_pool,
										GPOS_NEW(m_memory_pool) CDXLPhysicalCTEProducer(m_memory_pool, ulCTEId, pdrgpulColIds),
										pdxlnPrL,
										child_dxlnode
										);

	pdxlnCTEProducer->SetProperties(dxl_properties);

	return pdxlnCTEProducer;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCTEConsumer
//
//	@doc:
//		Translate a physical cte consumer expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnCTEConsumer
	(
	CExpression *pexprCTEConsumer,
	DrgPcr *, //pdrgpcr,
	DrgPds *, // pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	GPOS_ASSERT(NULL != pexprCTEConsumer);

	// extract components
	CPhysicalCTEConsumer *popCTEConsumer = CPhysicalCTEConsumer::PopConvert(pexprCTEConsumer->Pop());

	// extract physical properties from cte consumer
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprCTEConsumer);

	// extract the CTE id and the array of colids
	const ULONG ulCTEId = popCTEConsumer->UlCTEId();
	DrgPcr *pdrgpcr = popCTEConsumer->Pdrgpcr();
	ULongPtrArray *pdrgpulColIds = CUtils::Pdrgpul(m_memory_pool, pdrgpcr);

	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pdrgpcr);

	// translate relational child expression
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcr);

	CDXLNode *pdxlnCTEConsumer = GPOS_NEW(m_memory_pool) CDXLNode
										(
										m_memory_pool,
										GPOS_NEW(m_memory_pool) CDXLPhysicalCTEConsumer(m_memory_pool, ulCTEId, pdrgpulColIds),
										pdxlnPrL
										);

	pcrsOutput->Release();

	pdxlnCTEConsumer->SetProperties(dxl_properties);

	return pdxlnCTEConsumer;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAppend
//
//	@doc:
//		Create a DXL Append node from an optimizer an union all node
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAppend
	(
	CExpression *pexprUnionAll,
	DrgPcr *, //pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprUnionAll);

	CPhysicalUnionAll *popUnionAll = CPhysicalUnionAll::PopConvert(pexprUnionAll->Pop());
	DrgPcr *pdrgpcrOutput = popUnionAll->PdrgpcrOutput();

	CDXLPhysicalAppend *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalAppend(m_memory_pool, false, false);
	CDXLNode *pdxlnAppend = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// set properties
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprUnionAll);
	pdxlnAppend->SetProperties(dxl_properties);

	// translate project list
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pdrgpcrOutput);

	// the append node does not re-order or trim it input or output columns. The trimming
	// and re-ordering of its output columns has to be done above it (if needed)
	// via a separate result node
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrOutput);
	pcrsOutput->Release();
	pcrsOutput = NULL;

	pdxlnAppend->AddChild(pdxlnPrL);

	// scalar condition
	CDXLNode *pdxlnCond = NULL;
	CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);
	pdxlnAppend->AddChild(filter_dxlnode);

	// translate children
	DrgDrgPcr *pdrgpdrgpcrInput = popUnionAll->PdrgpdrgpcrInput();
	GPOS_ASSERT(NULL != pdrgpdrgpcrInput);
	const ULONG ulLen = pexprUnionAll->Arity();
	GPOS_ASSERT(ulLen == pdrgpdrgpcrInput->Size());
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		// translate child
		DrgPcr *pdrgpcrInput = (*pdrgpdrgpcrInput)[ul];

		CExpression *pexprChild = (*pexprUnionAll)[ul];
		CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcrInput, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

		// add a result node on top if necessary so the order of the input project list
		// matches the order in which the append node requires it
		CDXLNode *pdxlnChildProjected = PdxlnRemapOutputColumns
											(
											pexprChild,
											child_dxlnode,
											pdrgpcrInput /* required input columns */,
											pdrgpcrInput /* order of the input columns */
											);

		pdxlnAppend->AddChild(pdxlnChildProjected);
	}

	return pdxlnAppend;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdrgpcrMerge
//
//	@doc: 
//		Combines the ordered columns and required columns into a single list
//      with members in the ordered list inserted before the remaining columns in 
//		required list. For instance, if the order list is (c, d) and 
//		the required list is (a, b, c, d) then the combined list is (c, d, a, b)
//---------------------------------------------------------------------------
DrgPcr *
CTranslatorExprToDXL::PdrgpcrMerge
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrOrder,
	DrgPcr *pdrgpcrRequired
	)
{
	CColRefSet *pcrsOutput = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	DrgPcr *pdrgpcrMerge = GPOS_NEW(memory_pool) DrgPcr(memory_pool);

	if (NULL != pdrgpcrOrder)
	{
		const ULONG ulLenOrder = pdrgpcrOrder->Size();
		for (ULONG ul = 0; ul < ulLenOrder; ul++)
		{
			CColRef *pcr = (*pdrgpcrOrder)[ul];
			pdrgpcrMerge->Append(pcr);
		}
		pcrsOutput->Include(pdrgpcrMerge);
	}

	const ULONG ulLenReqd = pdrgpcrRequired->Size();
	for (ULONG ul = 0; ul < ulLenReqd; ul++)
	{
		CColRef *pcr = (*pdrgpcrRequired)[ul];
		if (!pcrsOutput->FMember(pcr))
		{
			pcrsOutput->Include(pcr);
			pdrgpcrMerge->Append(pcr);
		}
	}
	
	pcrsOutput->Release();

	return pdrgpcrMerge;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRemapOutputColumns
//
//	@doc:
//		Checks if the project list of the given node matches the required
//		columns and their order. If not then a result node is created on
//		top of it
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnRemapOutputColumns
	(
	CExpression *pexpr,
	CDXLNode *pdxln,
	DrgPcr *pdrgpcrRequired,
	DrgPcr *pdrgpcrOrder
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(NULL != pdrgpcrRequired);

	// get project list
	CDXLNode *pdxlnPrL = (*pdxln)[0];

	DrgPcr *pdrgpcrOrderedReqdCols = PdrgpcrMerge(m_memory_pool, pdrgpcrOrder, pdrgpcrRequired);
	
	// if the combined list is the same as proj list then no
	// further action needed. Otherwise we need result node on top
	if (CTranslatorExprToDXLUtils::FProjectListMatch(pdxlnPrL, pdrgpcrOrderedReqdCols))
	{
		pdrgpcrOrderedReqdCols->Release();
		return pdxln;
	}

	pdrgpcrOrderedReqdCols->Release();
	
	// output columns of new result node
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pdrgpcrRequired);

	CDXLNode *pdxlnPrLNew = PdxlnProjList(pcrsOutput, pdrgpcrOrder);
	pcrsOutput->Release();

	// create a result node on top of the current dxl node with a new project list
	return PdxlnResult
				(
				GetProperties(pexpr),
				pdxlnPrLNew,
				pdxln
				);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTVF
//
//	@doc:
//		Create a DXL TVF node from an optimizer TVF node
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnTVF
	(
	CExpression *pexprTVF,
	DrgPcr *, //pdrgpcr,
	DrgPds *, // pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	GPOS_ASSERT(NULL != pexprTVF);

	CPhysicalTVF *popTVF = CPhysicalTVF::PopConvert(pexprTVF->Pop());

	CColRefSet *pcrsOutput = popTVF->PcrsOutput();

	IMDId *mdid_func = popTVF->FuncMdId();
	mdid_func->AddRef();

	IMDId *mdid_return_type = popTVF->ReturnTypeMdId();
	mdid_return_type->AddRef();

	CWStringConst *pstrFunc = GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, popTVF->Pstr()->GetBuffer());

	CDXLPhysicalTVF *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalTVF(m_memory_pool, mdid_func, mdid_return_type, pstrFunc);

	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprTVF);
	CDXLNode *pdxlnTVF = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	pdxlnTVF->SetProperties(dxl_properties);
	
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, NULL /*pdrgpcr*/);
	pdxlnTVF->AddChild(pdxlnPrL); 		// project list

	TranslateScalarChildren(pexprTVF, pdxlnTVF);

	return pdxlnTVF;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromConstTableGet
//
//	@doc:
//		Create a DXL result node from an optimizer const table get node
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResultFromConstTableGet
	(
	CExpression *pexprCTG,
	DrgPcr *pdrgpcr,
	CExpression *pexprScalar
	)
{
	GPOS_ASSERT(NULL != pexprCTG);
	
	CPhysicalConstTableGet *popCTG = CPhysicalConstTableGet::PopConvert(pexprCTG->Pop());
	
	// construct project list from the const table get values
	DrgPcr *pdrgpcrCTGOutput = popCTG->PdrgpcrOutput();
	DrgPdrgPdatum *pdrgpdrgdatum = popCTG->Pdrgpdrgpdatum();
	
	const ULONG ulRows = pdrgpdrgdatum->Size();
	CDXLNode *pdxlnPrL = NULL;
	CDXLNode *pdxlnOneTimeFilter = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool));

	DrgPdatum *pdrgpdatum = NULL;
	if (0 == ulRows)
	{
		// no-tuples... only generate one row of NULLS and one-time "false" filter
		pdrgpdatum = CTranslatorExprToDXLUtils::PdrgpdatumNulls(m_memory_pool, pdrgpcrCTGOutput);

		CExpression *pexprFalse = CUtils::PexprScalarConstBool(m_memory_pool, false /*value*/, false /*is_null*/);
		CDXLNode *pdxlnFalse = PdxlnScConst(pexprFalse);
		pexprFalse->Release();

		pdxlnOneTimeFilter->AddChild(pdxlnFalse);
	}
	else
	{
		GPOS_ASSERT(1 <= ulRows);
		pdrgpdatum = (*pdrgpdrgdatum)[0];
		pdrgpdatum->AddRef();
		CDXLNode *pdxlnCond = NULL;
		if (NULL != pexprScalar)
		{
			pdxlnCond = PdxlnScalar(pexprScalar);
			pdxlnOneTimeFilter->AddChild(pdxlnCond);
		}
	}

	// if CTG has multiple rows then it has to be a valuescan of constants,
	// else, a Result node is created
	if (ulRows > 1)
	{
		GPOS_ASSERT(NULL != pdrgpcrCTGOutput);

		CColRefSet *pcrsOutput = CDrvdPropRelational::Pdprel(pexprCTG->Pdp(CDrvdProp::EptRelational))->PcrsOutput();
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrCTGOutput);

		CDXLNode *pdxlnValuesScan = CTranslatorExprToDXLUtils::PdxlnValuesScan
																(
																m_memory_pool,
																GetProperties(pexprCTG),
																pdxlnPrL,
																pdrgpdrgdatum
																);
		pdxlnOneTimeFilter->Release();
		pdrgpdatum->Release();

		return pdxlnValuesScan;
	}
	else
	{
		pdxlnPrL = PdxlnProjListFromConstTableGet(pdrgpcr, pdrgpcrCTGOutput, pdrgpdatum);
		pdrgpdatum->Release();
		return CTranslatorExprToDXLUtils::PdxlnResult
											(
											m_memory_pool,
											GetProperties(pexprCTG),
											pdxlnPrL,
											PdxlnFilter(NULL),
											pdxlnOneTimeFilter,
											NULL //child_dxlnode
											);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromConstTableGet
//
//	@doc:
//		Create a DXL result node from an optimizer const table get node
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResultFromConstTableGet
	(
	CExpression *pexprCTG,
	DrgPcr *pdrgpcr,
	DrgPds *, // pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	BOOL * // pfDML
	)
{
	return PdxlnResultFromConstTableGet(pexprCTG, pdrgpcr, NULL /*pexprScalarCond*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnComputeScalar
//
//	@doc:
//		Create a DXL result node from an optimizer compute scalar expression
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnComputeScalar
	(
	CExpression *pexprComputeScalar,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprComputeScalar);

	// extract components
	CExpression *pexprRelational = (*pexprComputeScalar)[0];
	CExpression *pexprProjList = (*pexprComputeScalar)[1];

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprRelational, NULL /* pdrgpcr */, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/ );

	// compute required columns
	GPOS_ASSERT(NULL != pexprComputeScalar->Prpp());
	CColRefSet *pcrsOutput = pexprComputeScalar->Prpp()->PcrsRequired();

	// iterate the columns in the projection list, add the columns containing
	// set-returning functions to the output columns
	const ULONG ulPrLs = pexprProjList->Arity();
	for (ULONG ul = 0; ul < ulPrLs; ul++)
	{
		CExpression *pexprPrE = (*pexprProjList)[ul];
		CDrvdPropScalar *pdpscalar = CDrvdPropScalar::Pdpscalar(pexprPrE->PdpDerive());

		// for column that doesn't contain set-returning function, if it is not the
		// required column in the relational plan properties, then no need to add them
		// to the output columns
		if (pdpscalar->FHasNonScalarFunction())
		{
			CScalarProjectElement *popScPrE = CScalarProjectElement::PopConvert(pexprPrE->Pop());
			pcrsOutput->Include(popScPrE->Pcr());
		}
	}

	// translate project list expression
	CDXLNode *pdxlnPrL = NULL;
	if (NULL == pdrgpcr || CUtils::FHasDuplicates(pdrgpcr))
	{
		pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput);
	}
	else
	{
		pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput, pdrgpcr);
	}

	// construct a result node
	CDXLNode *pdxlnResult = PdxlnResult
					(
					 GetProperties(pexprComputeScalar),
					 pdxlnPrL,
					 child_dxlnode
					 );

#ifdef GPOS_DEBUG
	(void) CDXLPhysicalResult::Cast(pdxlnResult->GetOperator())->AssertValid(pdxlnResult, false /* validate_children */);
#endif
	return pdxlnResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAggregate
//
//	@doc:
//		Create a DXL aggregate node from an optimizer hash agg expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAggregate
	(
	CExpression *pexprAgg,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprAgg);
	COperator::EOperatorId eopid = pexprAgg->Pop()->Eopid();
	
	// extract components and construct an aggregate node
	CPhysicalAgg *popAgg = NULL;

	GPOS_ASSERT(COperator::EopPhysicalStreamAgg == eopid ||
				COperator::EopPhysicalHashAgg == eopid ||
				COperator::EopPhysicalScalarAgg == eopid);

	EdxlAggStrategy agg_strategy_dxl = EdxlaggstrategySentinel;

	switch (eopid)
	{
		case COperator::EopPhysicalStreamAgg:
						{
							popAgg = CPhysicalStreamAgg::PopConvert(pexprAgg->Pop());
							agg_strategy_dxl = EdxlaggstrategySorted;
							break;
						}
		case COperator::EopPhysicalHashAgg:
						{
							popAgg = CPhysicalHashAgg::PopConvert(pexprAgg->Pop());
							agg_strategy_dxl = EdxlaggstrategyHashed;
							break;
						}
		case COperator::EopPhysicalScalarAgg:
						{
							popAgg = CPhysicalScalarAgg::PopConvert(pexprAgg->Pop());
							agg_strategy_dxl = EdxlaggstrategyPlain;
							break;
						}
		default:
			{
				return NULL;	// to silence the compiler
			}
	}

	const DrgPcr *pdrgpcrGroupingCols = popAgg->PdrgpcrGroupingCols();

	return PdxlnAggregate
			(
			pexprAgg,
			pdrgpcr, 
			pdrgpdsBaseTables, 
			pulNonGatherMotions,
			pfDML,
			agg_strategy_dxl,
			pdrgpcrGroupingCols,
			NULL /*pcrsKeys*/
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAggregateDedup
//
//	@doc:
//		Create a DXL aggregate node from an optimizer dedup agg expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAggregateDedup
	(
	CExpression *pexprAgg,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprAgg);
	COperator::EOperatorId eopid = pexprAgg->Pop()->Eopid();

	GPOS_ASSERT(COperator::EopPhysicalStreamAggDeduplicate == eopid ||
				COperator::EopPhysicalHashAggDeduplicate == eopid);

	EdxlAggStrategy agg_strategy_dxl = EdxlaggstrategySentinel;
	const DrgPcr *pdrgpcrGroupingCols = NULL;
	CColRefSet *pcrsKeys = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);

	if (COperator::EopPhysicalStreamAggDeduplicate == eopid)
	{
		CPhysicalStreamAggDeduplicate *popAggDedup = CPhysicalStreamAggDeduplicate::PopConvert(pexprAgg->Pop());
		pcrsKeys->Include(popAggDedup->PdrgpcrKeys());
		pdrgpcrGroupingCols = popAggDedup->PdrgpcrGroupingCols();
		agg_strategy_dxl = EdxlaggstrategySorted;
	}
	else
	{
		CPhysicalHashAggDeduplicate *popAggDedup = CPhysicalHashAggDeduplicate::PopConvert(pexprAgg->Pop());
		pcrsKeys->Include(popAggDedup->PdrgpcrKeys());
		pdrgpcrGroupingCols = popAggDedup->PdrgpcrGroupingCols();
		agg_strategy_dxl = EdxlaggstrategyHashed;
	}

	CDXLNode *pdxlnAgg = PdxlnAggregate
							(
							pexprAgg,
							pdrgpcr,
							pdrgpdsBaseTables, 
							pulNonGatherMotions,
							pfDML,
							agg_strategy_dxl,
							pdrgpcrGroupingCols,
							pcrsKeys
							);
	pcrsKeys->Release();

	return pdxlnAgg;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAggregate
//
//	@doc:
//		Create a DXL aggregate node from an optimizer agg expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAggregate
	(
	CExpression *pexprAgg,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	EdxlAggStrategy agg_strategy_dxl,
	const DrgPcr *pdrgpcrGroupingCols,
	CColRefSet *pcrsKeys
	)
{
	GPOS_ASSERT(NULL != pexprAgg);
	GPOS_ASSERT(NULL != pdrgpcrGroupingCols);
#ifdef GPOS_DEBUG
	COperator::EOperatorId eopid = pexprAgg->Pop()->Eopid();
	GPOS_ASSERT_IMP(NULL == pcrsKeys, COperator::EopPhysicalStreamAgg == eopid ||
									COperator::EopPhysicalHashAgg == eopid ||
									COperator::EopPhysicalScalarAgg == eopid);
#endif //GPOS_DEBUG

	// is it safe to stream the local hash aggregate
	BOOL stream_safe = CTranslatorExprToDXLUtils::FLocalHashAggStreamSafe(pexprAgg);

	CExpression *pexprChild = (*pexprAgg)[0];
	CExpression *pexprProjList = (*pexprAgg)[1];

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode
							(
							pexprChild, 
							NULL, // pdrgpcr, 
							pdrgpdsBaseTables,  
							pulNonGatherMotions, 
							pfDML, 
							false, // fRemap, 
							false // fRoot
							);

	// compute required columns
	GPOS_ASSERT(NULL != pexprAgg->Prpp());
	CColRefSet *pcrsRequired = pexprAgg->Prpp()->PcrsRequired();

	// translate project list expression
	CDXLNode *proj_list_dxlnode = PdxlnProjList(pexprProjList, pcrsRequired, pdrgpcr);

	// create an empty filter
	CDXLNode *filter_dxlnode = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarFilter(m_memory_pool));

	// construct grouping columns list and check if all the grouping column are
	// already in the project list of the aggregate operator

	const ULONG ulCols = proj_list_dxlnode->Arity();
	HMUlUl *phmululPL = GPOS_NEW(m_memory_pool) HMUlUl(m_memory_pool);
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CDXLNode *pdxlnProjElem = (*proj_list_dxlnode)[ul];
		ULONG col_id = CDXLScalarProjElem::Cast(pdxlnProjElem->GetOperator())->UlId();

		if (NULL == phmululPL->Find(&col_id))
		{
#ifdef GPOS_DEBUG
			BOOL fRes =
#endif
			phmululPL->Insert(GPOS_NEW(m_memory_pool) ULONG(col_id), GPOS_NEW(m_memory_pool) ULONG(col_id));
			GPOS_ASSERT(fRes);
		}
	}

	ULongPtrArray *pdrgpulGroupingCols = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);

	const ULONG ulLen = pdrgpcrGroupingCols->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CColRef *pcrGroupingCol = (*pdrgpcrGroupingCols)[ul];

		// only add columns that are either required or in the join keys.
		// if the keys colrefset is null, then skip this check
		if (NULL != pcrsKeys &&
			!pcrsKeys->FMember(pcrGroupingCol) &&
			!pcrsRequired->FMember(pcrGroupingCol))
		{
			continue;
		}

		pdrgpulGroupingCols->Append(GPOS_NEW(m_memory_pool) ULONG(pcrGroupingCol->UlId()));

		ULONG col_id = pcrGroupingCol->UlId();
		if (NULL == phmululPL->Find(&col_id))
		{
			CDXLNode *pdxlnProjElem = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcrGroupingCol);
			proj_list_dxlnode->AddChild(pdxlnProjElem);
#ifdef GPOS_DEBUG
		BOOL fRes =
#endif
				phmululPL->Insert(GPOS_NEW(m_memory_pool) ULONG(col_id), GPOS_NEW(m_memory_pool) ULONG(col_id));
			GPOS_ASSERT(fRes);
		}
	}
	
	phmululPL->Release();

	CDXLPhysicalAgg *pdxlopAgg = GPOS_NEW(m_memory_pool) CDXLPhysicalAgg(m_memory_pool, agg_strategy_dxl, stream_safe);
	pdxlopAgg->SetGroupingCols(pdrgpulGroupingCols);

	CDXLNode *pdxlnAgg = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopAgg);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprAgg);
	pdxlnAgg->SetProperties(dxl_properties);

	// add children
	pdxlnAgg->AddChild(proj_list_dxlnode);
	pdxlnAgg->AddChild(filter_dxlnode);
	pdxlnAgg->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopAgg->AssertValid(pdxlnAgg, false /* validate_children */);
#endif

	return pdxlnAgg;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSort
//
//	@doc:
//		Create a DXL sort node from an optimizer physical sort expression
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnSort
	(
	CExpression *pexprSort,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprSort);
	
	GPOS_ASSERT(1 == pexprSort->Arity());

	// extract components
	CPhysicalSort *popSort = CPhysicalSort::PopConvert(pexprSort->Pop());
	CExpression *pexprChild = (*pexprSort)[0];

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// translate order spec
	CDXLNode *sort_col_list_dxl = GetSortColListDXL(popSort->Pos());
	
	// construct project list from child project list
	GPOS_ASSERT(NULL != child_dxlnode && 1 <= child_dxlnode->Arity());
	CDXLNode *pdxlnProjListChild = (*child_dxlnode)[0];
	CDXLNode *proj_list_dxlnode = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// create an empty filter
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL);
	
	// construct a sort node
	CDXLPhysicalSort *pdxlopSort = GPOS_NEW(m_memory_pool) CDXLPhysicalSort(m_memory_pool, false /*fDiscardDuplicates*/);
	
	// construct sort node from its components
	CDXLNode *pdxlnSort = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopSort);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprSort);
	pdxlnSort->SetProperties(dxl_properties);

	// construct empty limit count and offset nodes
	CDXLNode *pdxlnLimitCount = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarLimitCount(m_memory_pool));
	CDXLNode *pdxlnLimitOffset = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarLimitOffset(m_memory_pool));
	
	// add children
	pdxlnSort->AddChild(proj_list_dxlnode);
	pdxlnSort->AddChild(filter_dxlnode);
	pdxlnSort->AddChild(sort_col_list_dxl);
	pdxlnSort->AddChild(pdxlnLimitCount);
	pdxlnSort->AddChild(pdxlnLimitOffset);
	pdxlnSort->AddChild(child_dxlnode);
	
#ifdef GPOS_DEBUG
	pdxlopSort->AssertValid(pdxlnSort, false /* validate_children */);
#endif
	
	return pdxlnSort;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnLimit
//
//	@doc:
//		Create a DXL limit node from an optimizer physical limit expression.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnLimit
	(
	CExpression *pexprLimit,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprLimit);
	GPOS_ASSERT(3 == pexprLimit->Arity());
	
	// extract components
	CExpression *pexprChild = (*pexprLimit)[0];
	CExpression *pexprOffset = (*pexprLimit)[1];
	CExpression *pexprCount = (*pexprLimit)[2];

	// bypass translation of limit if it does not have row count and offset
	CPhysicalLimit *popLimit = CPhysicalLimit::PopConvert(pexprLimit->Pop());
	if (!popLimit->FHasCount() && CUtils::FHasZeroOffset(pexprLimit))
	{
		return CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot*/);
	}
	
	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot*/);

	// translate limit offset and count
	CDXLNode *pdxlnLimitOffset = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarLimitOffset(m_memory_pool));
	pdxlnLimitOffset->AddChild(PdxlnScalar(pexprOffset));
	
	CDXLNode *pdxlnLimitCount = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarLimitCount(m_memory_pool));
	pdxlnLimitCount->AddChild(PdxlnScalar(pexprCount));
	
	// construct project list from child project list
	GPOS_ASSERT(NULL != child_dxlnode && 1 <= child_dxlnode->Arity());
	CDXLNode *pdxlnProjListChild = (*child_dxlnode)[0];
	CDXLNode *proj_list_dxlnode = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// construct a limit node
	CDXLPhysicalLimit *pdxlopLimit = GPOS_NEW(m_memory_pool) CDXLPhysicalLimit(m_memory_pool);

	// construct limit node from its components
	CDXLNode *pdxlnLimit = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopLimit);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprLimit);
	pdxlnLimit->SetProperties(dxl_properties);
	
	pdxlnLimit->AddChild(proj_list_dxlnode);
	pdxlnLimit->AddChild(child_dxlnode);
	pdxlnLimit->AddChild(pdxlnLimitCount);
	pdxlnLimit->AddChild(pdxlnLimitOffset);
	
#ifdef GPOS_DEBUG
	pdxlopLimit->AssertValid(pdxlnLimit, false /* validate_children */);
#endif
	
	return pdxlnLimit;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildSubplansForCorrelatedLOJ
//
//	@doc:
//		Helper to build subplans from correlated LOJ
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildSubplansForCorrelatedLOJ
	(
	CExpression *pexprCorrelatedLOJ,
	DrgPdxlcr *pdrgdxlcr,
	CDXLNode **ppdxlnScalar, // output: scalar condition after replacing inner child reference with subplan
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprCorrelatedLOJ);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftOuterNLJoin == pexprCorrelatedLOJ->Pop()->Eopid());

	CExpression *pexprInner = (*pexprCorrelatedLOJ)[1];
	CExpression *pexprScalar = (*pexprCorrelatedLOJ)[2];

	DrgPcr *pdrgpcrInner = CPhysicalNLJoin::PopConvert(pexprCorrelatedLOJ->Pop())->PdrgPcrInner();
	GPOS_ASSERT(NULL != pdrgpcrInner);

	EdxlSubPlanType edxlsubplantype = Edxlsubplantype(pexprCorrelatedLOJ);

	if (EdxlSubPlanTypeScalar == edxlsubplantype)
	{
		// for correlated left outer join for scalar subplan type, we generate a scalar subplan
		BuildScalarSubplans(pdrgpcrInner, pexprInner, pdrgdxlcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

		// now translate the scalar - references to the inner child will be
		// replaced by the subplan
		*ppdxlnScalar = PdxlnScalar(pexprScalar);

		return;
	}

	GPOS_ASSERT
		(
		EdxlSubPlanTypeAny == edxlsubplantype ||
		EdxlSubPlanTypeAll == edxlsubplantype ||
		EdxlSubPlanTypeExists == edxlsubplantype ||
		EdxlSubPlanTypeNotExists == edxlsubplantype
		);

	// for correlated left outer join with non-scalar subplan type,
	// we need to generate quantified/exitential subplan
	if (EdxlSubPlanTypeAny == edxlsubplantype || EdxlSubPlanTypeAll == edxlsubplantype)
	{
		(void) PdxlnQuantifiedSubplan(pdrgpcrInner, pexprCorrelatedLOJ, pdrgdxlcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	}
	else
	{
		GPOS_ASSERT(EdxlSubPlanTypeExists == edxlsubplantype || EdxlSubPlanTypeNotExists == edxlsubplantype);
		(void) PdxlnExistentialSubplan(pdrgpcrInner, pexprCorrelatedLOJ, pdrgdxlcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	}

	CExpression *pexprTrue = CUtils::PexprScalarConstBool(m_memory_pool, true /*value*/, false /*is_null*/);
	*ppdxlnScalar = PdxlnScalar(pexprTrue);
	pexprTrue->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildSubplans
//
//	@doc:
//		Helper to build subplans of different types
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildSubplans
	(
	CExpression *pexprCorrelatedNLJoin,
	DrgPdxlcr *pdrgdxlcr,
	CDXLNode **ppdxlnScalar, // output: scalar condition after replacing inner child reference with subplan
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexprCorrelatedNLJoin->Pop()));
	GPOS_ASSERT(NULL != ppdxlnScalar);

	CExpression *pexprInner = (*pexprCorrelatedNLJoin)[1];
	CExpression *pexprScalar = (*pexprCorrelatedNLJoin)[2];

	DrgPcr *pdrgpcrInner = CPhysicalNLJoin::PopConvert(pexprCorrelatedNLJoin->Pop())->PdrgPcrInner();
	GPOS_ASSERT(NULL != pdrgpcrInner);

	COperator::EOperatorId eopid = pexprCorrelatedNLJoin->Pop()->Eopid();
	CDXLNode *pdxlnSubPlan = NULL;
	switch (eopid)
	{
		case COperator::EopPhysicalCorrelatedLeftOuterNLJoin:
			BuildSubplansForCorrelatedLOJ(pexprCorrelatedNLJoin, pdrgdxlcr, ppdxlnScalar, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
			return;

		case COperator::EopPhysicalCorrelatedInnerNLJoin:
			BuildScalarSubplans(pdrgpcrInner, pexprInner, pdrgdxlcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

			// now translate the scalar - references to the inner child will be
			// replaced by the subplan
			*ppdxlnScalar = PdxlnScalar(pexprScalar);
			return;

		case COperator::EopPhysicalCorrelatedInLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin:
			pdxlnSubPlan = PdxlnQuantifiedSubplan(pdrgpcrInner, pexprCorrelatedNLJoin, pdrgdxlcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
			pdxlnSubPlan->AddRef();
			*ppdxlnScalar = pdxlnSubPlan;
			return;

		case COperator::EopPhysicalCorrelatedLeftSemiNLJoin:
		case COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin:
			pdxlnSubPlan = PdxlnExistentialSubplan(pdrgpcrInner, pexprCorrelatedNLJoin, pdrgdxlcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
			pdxlnSubPlan->AddRef();
			*ppdxlnScalar = pdxlnSubPlan;
			return;

		default:
				GPOS_ASSERT(!"Unsupported correlated join");
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRestrictResult
//
//	@doc:
//		Helper to build a Result expression with project list
//		restricted to required column
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnRestrictResult
	(
	CDXLNode *pdxln,
	CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(NULL != pcr);

	CDXLNode *pdxlnProjListOld = (*pdxln)[0];
	const ULONG ulPrjElems = pdxlnProjListOld->Arity();

	if (0 == ulPrjElems)
	{
		// failed to find project elements
		pdxln->Release();
		return NULL;
	}

	CDXLNode *pdxlnResult = pdxln;
	if (1 < ulPrjElems)
	{
		// restrict project list to required column
		CDXLScalarProjList *pdxlopPrL = GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool);
		CDXLNode *pdxlnProjListNew = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopPrL);

		for (ULONG ul = 0; ul < ulPrjElems; ul++)
		{
			CDXLNode *child_dxlnode = (*pdxlnProjListOld)[ul];
			CDXLScalarProjElem *pdxlPrjElem = CDXLScalarProjElem::Cast(child_dxlnode->GetOperator());
			if (pdxlPrjElem->UlId() == pcr->UlId())
			{
				// create a new project element that simply points to required column,
				// we cannot re-use child_dxlnode here since it may have a deep expression with columns inaccessible
				// above the child (inner) DXL expression
				CDXLNode *pdxlnPrEl = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcr);
				pdxlnProjListNew->AddChild(pdxlnPrEl);
			}
		}
		GPOS_ASSERT(1 == pdxlnProjListNew->Arity());

		pdxlnResult = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLPhysicalResult(m_memory_pool));
		CDXLPhysicalProperties *dxl_properties = CTranslatorExprToDXLUtils::PdxlpropCopy(m_memory_pool, pdxln);
		pdxlnResult->SetProperties(dxl_properties);

		pdxlnResult->AddChild(pdxlnProjListNew);
		pdxlnResult->AddChild(PdxlnFilter(NULL));
		pdxlnResult->AddChild(GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool)));
		pdxlnResult->AddChild(pdxln);
	}

	return pdxlnResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnQuantifiedSubplan
//
//	@doc:
//		Helper to build subplans for quantified (ANY/ALL) subqueries
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnQuantifiedSubplan
	(
	DrgPcr *pdrgpcrInner,
	CExpression *pexprCorrelatedNLJoin,
	DrgPdxlcr *pdrgdxlcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	COperator *popCorrelatedJoin = pexprCorrelatedNLJoin->Pop();
	COperator::EOperatorId eopid = popCorrelatedJoin->Eopid();
	BOOL fCorrelatedLOJ = (COperator::EopPhysicalCorrelatedLeftOuterNLJoin == eopid);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedInLeftSemiNLJoin == eopid ||
			COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin == eopid ||
			fCorrelatedLOJ);

	EdxlSubPlanType edxlsubplantype = Edxlsubplantype(pexprCorrelatedNLJoin);
	GPOS_ASSERT_IMP(fCorrelatedLOJ, EdxlSubPlanTypeAny == edxlsubplantype || EdxlSubPlanTypeAll == edxlsubplantype);

	CExpression *pexprInner = (*pexprCorrelatedNLJoin)[1];
	CExpression *pexprScalar = (*pexprCorrelatedNLJoin)[2];

	// translate inner child
	CDXLNode *pdxlnInnerChild = CreateDXLNode(pexprInner, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// find required column from inner child
	CColRef *pcrInner = (*pdrgpcrInner)[0];

	if (fCorrelatedLOJ)
	{
		// overwrite required inner column based on scalar expression

		CColRefSet *pcrsInner = CDrvdPropRelational::Pdprel(pexprInner->Pdp(CDrvdProp::EptRelational))->PcrsOutput();
		CColRefSet *pcrsUsed = GPOS_NEW(m_memory_pool) CColRefSet (m_memory_pool, *CDrvdPropScalar::Pdpscalar(pexprScalar->Pdp(CDrvdProp::EptScalar))->PcrsUsed());
		pcrsUsed->Intersection(pcrsInner);
		if (0 < pcrsUsed->Size())
		{
			GPOS_ASSERT(1 == pcrsUsed->Size());

			pcrInner = pcrsUsed->PcrFirst();
		}
		pcrsUsed->Release();
	}

	CDXLNode *pdxlnInner = PdxlnRestrictResult(pdxlnInnerChild, pcrInner);
	if (NULL == pdxlnInner)
	{
		GPOS_RAISE(gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature, GPOS_WSZ_LIT("Outer references in the project list of a correlated subquery"));
	}

	// translate test expression
	CDXLNode *pdxlnTestExpr = PdxlnScalar(pexprScalar);

	const IMDTypeBool *pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	IMDId *pmdid = pmdtypebool->MDId();
	pmdid->AddRef();

	// construct a subplan node, with the inner child under it
	CDXLNode *pdxlnSubPlan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSubPlan(m_memory_pool, pmdid, pdrgdxlcr, edxlsubplantype, pdxlnTestExpr));
	pdxlnSubPlan->AddChild(pdxlnInner);

	// add to hashmap
#ifdef GPOS_DEBUG
	BOOL fRes =
#endif // GPOS_DEBUG
		m_phmcrdxln->Insert(const_cast<CColRef *>((*pdrgpcrInner)[0]), pdxlnSubPlan);
	GPOS_ASSERT(fRes);

	return pdxlnSubPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjectBoolConst
//
//	@doc:
//		Helper to add a project of bool constant on top of given DXL node
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjectBoolConst
	(
	CDXLNode *pdxln,
	BOOL value
	)
{
	GPOS_ASSERT(NULL != pdxln);

	// create a new project element with bool value
	const IMDTypeBool *pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	IMDId *pmdid = pmdtypebool->MDId();
	pmdid->AddRef();

	CDXLDatumBool *datum_dxl = GPOS_NEW(m_memory_pool) CDXLDatumBool(m_memory_pool, pmdid, false /* is_null */,  value);
	CDXLScalarConstValue *pdxlopConstValue = GPOS_NEW(m_memory_pool) CDXLScalarConstValue(m_memory_pool, datum_dxl);
	CColRef *pcr = m_pcf->PcrCreate(pmdtypebool, IDefaultTypeModifier);
	CDXLNode *pdxlnPrEl = PdxlnProjElem(pcr, GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopConstValue));

	CDXLScalarProjList *pdxlopPrL = GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool);
	CDXLNode *proj_list_dxlnode = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopPrL);
	proj_list_dxlnode->AddChild(pdxlnPrEl);
	CDXLNode *pdxlnResult = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLPhysicalResult(m_memory_pool));
	CDXLPhysicalProperties *dxl_properties = CTranslatorExprToDXLUtils::PdxlpropCopy(m_memory_pool, pdxln);
	pdxlnResult->SetProperties(dxl_properties);

	pdxlnResult->AddChild(proj_list_dxlnode);
	pdxlnResult->AddChild(PdxlnFilter(NULL));
	pdxlnResult->AddChild(GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool)));
	pdxlnResult->AddChild(pdxln);

	return pdxlnResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::EdxlsubplantypeCorrelatedLOJ
//
//	@doc:
//		Helper to find subplan type from a correlated left outer
//		join expression
//
//---------------------------------------------------------------------------
EdxlSubPlanType
CTranslatorExprToDXL::EdxlsubplantypeCorrelatedLOJ
	(
	CExpression *pexprCorrelatedLOJ
	)
{
	GPOS_ASSERT(NULL != pexprCorrelatedLOJ);
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftOuterNLJoin == pexprCorrelatedLOJ->Pop()->Eopid());

	COperator::EOperatorId eopidSubq =
			CPhysicalCorrelatedLeftOuterNLJoin::PopConvert(pexprCorrelatedLOJ->Pop())->EopidOriginSubq();
	switch (eopidSubq)
	{
		case COperator::EopScalarSubquery:
			return EdxlSubPlanTypeScalar;

		case COperator::EopScalarSubqueryAll:
			return EdxlSubPlanTypeAll;

		case COperator::EopScalarSubqueryAny:
			return EdxlSubPlanTypeAny;

		case COperator::EopScalarSubqueryExists:
			return EdxlSubPlanTypeExists;

		case COperator::EopScalarSubqueryNotExists:
			return  EdxlSubPlanTypeNotExists;

		default:
			GPOS_ASSERT(!"Unexpected origin subquery in correlated left outer join");
	}

	return EdxlSubPlanTypeSentinel;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Edxlsubplantype
//
//	@doc:
//		Helper to find subplan type from a correlated join expression
//
//---------------------------------------------------------------------------
EdxlSubPlanType
CTranslatorExprToDXL::Edxlsubplantype
	(
	CExpression *pexprCorrelatedNLJoin
	)
{
	GPOS_ASSERT(NULL != pexprCorrelatedNLJoin);
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexprCorrelatedNLJoin->Pop()));

	COperator::EOperatorId eopid = pexprCorrelatedNLJoin->Pop()->Eopid();
	switch (eopid)
	{
		case COperator::EopPhysicalCorrelatedLeftOuterNLJoin:
			return EdxlsubplantypeCorrelatedLOJ(pexprCorrelatedNLJoin);

		case COperator::EopPhysicalCorrelatedInnerNLJoin:
			return EdxlSubPlanTypeScalar;

		case COperator::EopPhysicalCorrelatedNotInLeftAntiSemiNLJoin:
			return EdxlSubPlanTypeAll;

		case COperator::EopPhysicalCorrelatedInLeftSemiNLJoin:
			return EdxlSubPlanTypeAny;

		case COperator::EopPhysicalCorrelatedLeftSemiNLJoin:
			return EdxlSubPlanTypeExists;

		case COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin:
			return EdxlSubPlanTypeNotExists;

		default:
			GPOS_ASSERT(!"Unexpected correlated join");
	}

	return EdxlSubPlanTypeSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnExistentialSubplan
//
//	@doc:
//		Helper to build subplans for existential subqueries
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnExistentialSubplan
	(
	DrgPcr *pdrgpcrInner,
	CExpression *pexprCorrelatedNLJoin,
	DrgPdxlcr *pdrgdxlcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
#ifdef GPOS_DEBUG
	COperator::EOperatorId eopid = pexprCorrelatedNLJoin->Pop()->Eopid();
	BOOL fCorrelatedLOJ = (COperator::EopPhysicalCorrelatedLeftOuterNLJoin == eopid);
#endif // GPOS_DEBUG
	GPOS_ASSERT(COperator::EopPhysicalCorrelatedLeftSemiNLJoin == eopid ||
			COperator::EopPhysicalCorrelatedLeftAntiSemiNLJoin == eopid ||
			fCorrelatedLOJ);

	EdxlSubPlanType edxlsubplantype = Edxlsubplantype(pexprCorrelatedNLJoin);
	GPOS_ASSERT_IMP(fCorrelatedLOJ, EdxlSubPlanTypeExists == edxlsubplantype || EdxlSubPlanTypeNotExists == edxlsubplantype);

	// translate inner child
	CExpression *pexprInner = (*pexprCorrelatedNLJoin)[1];
	
	CDXLNode *pdxlnInnerChild = CreateDXLNode(pexprInner, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	CDXLNode *pdxlnInnerProjList = (*pdxlnInnerChild)[0];
	CDXLNode *pdxlnInner = NULL;
	if (0 == pdxlnInnerProjList->Arity())
	{
		// no requested columns from subplan, add a dummy boolean constant to project list
		pdxlnInner = PdxlnProjectBoolConst(pdxlnInnerChild, true /*value*/);
	}
	else
	{
		// restrict requested columns to required inner column
		pdxlnInner = PdxlnRestrictResult(pdxlnInnerChild, (*pdrgpcrInner)[0]);
	}
	
	if (NULL == pdxlnInner)
	{
		GPOS_RAISE(gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature, GPOS_WSZ_LIT("Outer references in the project list of a correlated subquery"));
	}

	const IMDTypeBool *pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	IMDId *pmdid = pmdtypebool->MDId();
	pmdid->AddRef();

	// construct a subplan node, with the inner child under it
	CDXLNode *pdxlnSubPlan =
		GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSubPlan(m_memory_pool, pmdid, pdrgdxlcr, edxlsubplantype, NULL /*pdxlnTestExpr*/));
	pdxlnSubPlan->AddChild(pdxlnInner);

	// add to hashmap
#ifdef GPOS_DEBUG
	BOOL fRes =
#endif // GPOS_DEBUG
		m_phmcrdxln->Insert(const_cast<CColRef *>((*pdrgpcrInner)[0]), pdxlnSubPlan);
	GPOS_ASSERT(fRes);

	return pdxlnSubPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildScalarSubplans
//
//	@doc:
//		Helper to build subplans from inner column references and store
//		generated subplans in subplan map
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildScalarSubplans
	(
	DrgPcr *pdrgpcrInner,
	CExpression *pexprInner,
	DrgPdxlcr *pdrgdxlcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	const ULONG ulSize = pdrgpcrInner->Size();

	DXLNodeArray *pdrgpdxlnInner = GPOS_NEW(m_memory_pool) DXLNodeArray(m_memory_pool);
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		// for each subplan, we need to re-translate inner expression
		CDXLNode *pdxlnInnerChild = CreateDXLNode(pexprInner, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
		CDXLNode *pdxlnInner = PdxlnRestrictResult(pdxlnInnerChild, (*pdrgpcrInner)[ul]);
		if (NULL == pdxlnInner)
		{
			GPOS_RAISE(gpopt::ExmaDXL, gpopt::ExmiExpr2DXLUnsupportedFeature, GPOS_WSZ_LIT("Outer references in the project list of a correlated subquery"));
		}
		pdrgpdxlnInner->Append(pdxlnInner);
	}

	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CDXLNode *pdxlnInner = (*pdrgpdxlnInner)[ul];
		pdxlnInner->AddRef();
		if (0 < ul)
		{
			// if there is more than one subplan, we need to add-ref passed arrays
			pdrgdxlcr->AddRef();
		}
		const CColRef *pcrInner = (*pdrgpcrInner)[ul];
		BuildDxlnSubPlan(pdxlnInner, pcrInner, pdrgdxlcr);
	}

	pdrgpdxlnInner->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PcrsOuterRefsForCorrelatedNLJoin
//
//	@doc:
//		Return outer refs in correlated join inner child
//
//---------------------------------------------------------------------------
CColRefSet *
CTranslatorExprToDXL::PcrsOuterRefsForCorrelatedNLJoin
	(
	CExpression *pexpr
	)
	const
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexpr->Pop()));

	CExpression *pexprInnerChild = (*pexpr)[1];

	// get inner child's relational properties
	CDrvdPropRelational *pdprel = CDrvdPropRelational::Pdprel(pexprInnerChild->Pdp(CDrvdProp::EptRelational));

	return pdprel->PcrsOuter();
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCorrelatedNLJoin
//
//	@doc:
//		Translate correlated NLJ expression.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnCorrelatedNLJoin
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(CUtils::FCorrelatedNLJoin(pexpr->Pop()));

	// extract components
	CExpression *pexprOuterChild = (*pexpr)[0];
	CExpression *pexprInnerChild = (*pexpr)[1];
	CExpression *pexprScalar = (*pexpr)[2];

	// outer references in the inner child
	DrgPdxlcr *pdrgdxlcr = GPOS_NEW(m_memory_pool) DrgPdxlcr(m_memory_pool);

	CColRefSet *pcrsOuter = PcrsOuterRefsForCorrelatedNLJoin(pexpr);
	CColRefSetIter crsi(*pcrsOuter);
	while (crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		CMDName *mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pcr->Name().Pstr());
		IMDId *pmdid = pcr->Pmdtype()->MDId();
		pmdid->AddRef();
		CDXLColRef *dxl_colref = GPOS_NEW(m_memory_pool) CDXLColRef(m_memory_pool, mdname, pcr->UlId(), pmdid, pcr->TypeModifier());
		pdrgdxlcr->Append(dxl_colref);
	}

	COperator::EOperatorId eopid = pexpr->Pop()->Eopid();
	CDXLNode *pdxlnCond = NULL;

    // Create a subplan with a Boolean from the inner child if we have a Const True as a join condition.
    // One scenario for this is when IN sublinks contain a projection from the outer table only such as:
    // select * from foo where foo.a in (select foo.b from bar);
    // If bar is a very small table, ORCA generates a CorrelatedInLeftSemiNLJoin with a Const true join filter
    // and condition foo.a = foo.b is added as a filter on the table scan of foo. If bar is a large table,
    // ORCA generates a plan with CorrelatedInnerNLJoin with a Const true join filter and a LIMIT over the
    // scan of bar. The same foo.a = foo.b condition is also added as a filter on the table scan of foo.
	if (CUtils::FScalarConstTrue(pexprScalar) &&
		(COperator::EopPhysicalCorrelatedInnerNLJoin == eopid || COperator::EopPhysicalCorrelatedInLeftSemiNLJoin == eopid))
	{
		// translate relational inner child expression
		CDXLNode *pdxlnInnerChild = CreateDXLNode
									(
									pexprInnerChild, 
									NULL, // pdrgpcr, 
									pdrgpdsBaseTables, 
									pulNonGatherMotions, 
									pfDML, 
									false, // fRemap
									false // fRoot
									);

		// if the filter predicate is a constant TRUE, create a subplan that returns
		// Boolean from the inner child, and use that as the scalar condition
		pdxlnCond = PdxlnBooleanScalarWithSubPlan(pdxlnInnerChild, pdrgdxlcr);
	}
	else
	{
		BuildSubplans(pexpr, pdrgdxlcr, &pdxlnCond, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	}

	// extract dxl properties from correlated join
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexpr);
	CDXLNode *pdxln = NULL;

	switch (pexprOuterChild->Pop()->Eopid())
	{
		case COperator::EopPhysicalTableScan:
		{
			dxl_properties->AddRef();
			// create and return a table scan node
			pdxln = PdxlnTblScanFromNLJoinOuter(pexprOuterChild, pdxlnCond, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, dxl_properties);
			break;
		}

		case COperator::EopPhysicalFilter:
		{
			dxl_properties->AddRef();
			pdxln = PdxlnResultFromNLJoinOuter(pexprOuterChild, pdxlnCond, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, dxl_properties);
			break;
		}

		default:
		{
			// create a result node over outer child
			dxl_properties->AddRef();
			pdxln = PdxlnResult(pexprOuterChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, pdxlnCond, dxl_properties);
		}
	}

	dxl_properties->Release();
	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::BuildDxlnSubPlan
//
//	@doc:
//		Construct a scalar dxl node with a subplan as its child. Also put this
//		subplan in the hashmap with its output column, so that anyone who
//		references that column can use the subplan
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::BuildDxlnSubPlan
	(
	CDXLNode *pdxlnRelChild,
	const CColRef *pcr,
	DrgPdxlcr *pdrgdxlcr
	)
{
	GPOS_ASSERT(NULL != pcr);
	IMDId *pmdid = pcr->Pmdtype()->MDId();
	pmdid->AddRef();

	// construct a subplan node, with the inner child under it
	CDXLNode *pdxlnSubPlan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSubPlan(m_memory_pool, pmdid, pdrgdxlcr, EdxlSubPlanTypeScalar, NULL));
	pdxlnSubPlan->AddChild(pdxlnRelChild);

	// add to hashmap
#ifdef GPOS_DEBUG
	BOOL fRes =
#endif // GPOS_DEBUG
	m_phmcrdxln->Insert(const_cast<CColRef *>(pcr), pdxlnSubPlan);
	GPOS_ASSERT(fRes);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnBooleanScalarWithSubPlan
//
//	@doc:
//		Construct a boolean scalar dxl node with a subplan as its child. The
//		sublan has a boolean output column, and has	the given relational child
//		under it
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnBooleanScalarWithSubPlan
	(
	CDXLNode *pdxlnRelChild,
	DrgPdxlcr *pdrgdxlcr
	)
{
	// create a new project element (const:true), and replace the first child with it
	const IMDTypeBool *pmdtypebool = m_pmda->PtMDType<IMDTypeBool>();
	IMDId *pmdid = pmdtypebool->MDId();
	pmdid->AddRef();

	CDXLDatumBool *datum_dxl = GPOS_NEW(m_memory_pool) CDXLDatumBool(m_memory_pool, pmdid, false /* is_null */, true /* value */);
	CDXLScalarConstValue *pdxlopConstValue = GPOS_NEW(m_memory_pool) CDXLScalarConstValue(m_memory_pool, datum_dxl);

	CColRef *pcr = m_pcf->PcrCreate(pmdtypebool, IDefaultTypeModifier);

	CDXLNode *pdxlnPrEl = PdxlnProjElem(pcr, GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopConstValue));

	// create a new Result node for the created project element
	CDXLNode *pdxlnProjListNew = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool));
	pdxlnProjListNew->AddChild(pdxlnPrEl);
	CDXLNode *pdxlnResult = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLPhysicalResult(m_memory_pool));
	CDXLPhysicalProperties *dxl_properties = CTranslatorExprToDXLUtils::PdxlpropCopy(m_memory_pool, pdxlnRelChild);
	pdxlnResult->SetProperties(dxl_properties);
	pdxlnResult->AddChild(pdxlnProjListNew);
	pdxlnResult->AddChild(PdxlnFilter(NULL));
	pdxlnResult->AddChild(GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool)));
	pdxlnResult->AddChild(pdxlnRelChild);

	// construct a subplan node, with the Result node under it
	pmdid->AddRef();
	CDXLNode *pdxlnSubPlan = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSubPlan(m_memory_pool, pmdid, pdrgdxlcr, EdxlSubPlanTypeScalar, NULL));
	pdxlnSubPlan->AddChild(pdxlnResult);

	return pdxlnSubPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScBoolExpr
//
//	@doc:
//		Create a DXL scalar boolean node given two DXL boolean nodes
//		and a boolean op
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScBoolExpr
	(
	EdxlBoolExprType boolexptype,
	CDXLNode *pdxlnLeft,
	CDXLNode *pdxlnRight
	)
{
	CDXLNode *pdxlnBoolExpr = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBoolExpr(m_memory_pool, boolexptype));

	pdxlnBoolExpr->AddChild(pdxlnLeft);
	pdxlnBoolExpr->AddChild(pdxlnRight);

	return pdxlnBoolExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnTblScanFromNLJoinOuter
//
//	@doc:
//		Create a DXL table scan node from the outer child of a NLJ
//		and a DXL scalar condition. Used for translated correlated
//		subqueries.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnTblScanFromNLJoinOuter
	(
	CExpression *pexprRelational,
	CDXLNode *pdxlnCond,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *, // pulNonGatherMotions,
	CDXLPhysicalProperties *dxl_properties
	)
{
	// create a table scan over the input expression, without a filter
	CDXLNode *pdxlnTblScan = PdxlnTblScan
								(
								pexprRelational,
								NULL, //pcrsOutput
								pdrgpcr,
								pdrgpdsBaseTables, 
								NULL, //pexprScalar
								dxl_properties
								);

	if (!CTranslatorExprToDXLUtils::FScalarConstTrue(m_pmda, pdxlnCond))
	{
		// add the new filter to the table scan replacing its original
		// empty filter
		CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnCond);
		pdxlnTblScan->ReplaceChild(EdxltsIndexFilter /*ulPos*/, filter_dxlnode);
	}
	else
	{
		// not used
		pdxlnCond->Release();
	}

	return pdxlnTblScan;
}

static ULONG
UlIndexFilter(Edxlopid edxlopid)
{
	switch (edxlopid)
	{
		case EdxlopPhysicalTableScan:
		case EdxlopPhysicalExternalScan:
			return EdxltsIndexFilter;
		case EdxlopPhysicalBitmapTableScan:
		case EdxlopPhysicalDynamicBitmapTableScan:
			return EdxlbsIndexFilter;
		case EdxlopPhysicalDynamicTableScan:
			return EdxldtsIndexFilter;
		case EdxlopPhysicalIndexScan:
		case EdxlopPhysicalDynamicIndexScan:
			return EdxlisIndexFilter;
		case EdxlopPhysicalResult:
			return EdxlresultIndexFilter;
		default:
			GPOS_RTL_ASSERT("Unexpected operator. Expected operators that contain a filter child");
			return ULONG_MAX;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnResultFromNLJoinOuter
//
//	@doc:
//		Create a DXL result node from the outer child of a NLJ
//		and a DXL scalar condition. Used for translated correlated
//		subqueries.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnResultFromNLJoinOuter
	(
	CExpression *pexprRelational,
	CDXLNode *pdxlnCond,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables, 
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CDXLPhysicalProperties *dxl_properties
	)
{
	// create a result node from the input expression
	CDXLNode *pdxlnResult = PdxlnResult(pexprRelational, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, dxl_properties);
	dxl_properties->Release();

	// In case the OuterChild is a physical sequence, it will already have the filter in the partition selector and
	// dynamic scan, thus we should not replace the filter.
	Edxlopid edxlopid = pdxlnResult->GetOperator()->GetDXLOperator();
	switch (edxlopid)
	{
		case EdxlopPhysicalTableScan:
		case EdxlopPhysicalExternalScan:
		case EdxlopPhysicalBitmapTableScan:
		case EdxlopPhysicalDynamicTableScan:
		case EdxlopPhysicalIndexScan:
		case EdxlopPhysicalDynamicIndexScan:
		case EdxlopPhysicalDynamicBitmapTableScan:
		case EdxlopPhysicalResult:
		{
			// get the original condition from the filter node
			// create a new AND expression
			ULONG ulIndexFilter = UlIndexFilter(edxlopid);
			GPOS_ASSERT(ulIndexFilter != ULONG_MAX);
			CDXLNode *pdxlnOrigFilter = (*pdxlnResult)[ulIndexFilter];
			GPOS_ASSERT(EdxlopScalarFilter == pdxlnOrigFilter->GetOperator()->GetDXLOperator());
			CDXLNode *pdxlnOrigCond = (*pdxlnOrigFilter)[0];
			pdxlnOrigCond->AddRef();

			CDXLNode *pdxlnBoolExpr = PdxlnScBoolExpr(Edxland, pdxlnOrigCond, pdxlnCond);

			// add the new filter to the result replacing its original
			// empty filter
			CDXLNode *filter_dxlnode = PdxlnFilter(pdxlnBoolExpr);
			pdxlnResult->ReplaceChild(ulIndexFilter /*ulPos*/, filter_dxlnode);
		}
			break;
		case EdxlopPhysicalSequence:
		{
			dxl_properties->AddRef();
			GPOS_ASSERT(NULL != pexprRelational->Prpp());
			CColRefSet *pcrsOutput = pexprRelational->Prpp()->PcrsRequired();
			pdxlnResult = PdxlnAddScalarFilterOnRelationalChild
							(
							pdxlnResult,
							pdxlnCond,
							dxl_properties,
							pcrsOutput,
							pdrgpcr
							);
		}
			break;
		default:
			pdxlnCond->Release();
			GPOS_RTL_ASSERT(false && "Unexpected node here");
	}

	return pdxlnResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::StoreIndexNLJOuterRefs
//
//	@doc:
//		Store outer references in index NLJ inner child into global map
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::StoreIndexNLJOuterRefs
	(
	CPhysical *pop
	)
{
	DrgPcr *pdrgpcr = NULL;

	if (COperator::EopPhysicalInnerIndexNLJoin == pop->Eopid())
	{
		pdrgpcr = CPhysicalInnerIndexNLJoin::PopConvert(pop)->PdrgPcrOuterRefs();
	}
	else
	{
		pdrgpcr = CPhysicalLeftOuterIndexNLJoin::PopConvert(pop)->PdrgPcrOuterRefs();
	}
	GPOS_ASSERT(pdrgpcr != NULL);

	const ULONG ulSize = pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		if (NULL == m_phmcrdxlnIndexLookup->Find(pcr))
		{
			CDXLNode *pdxln = CTranslatorExprToDXLUtils::PdxlnIdent(m_memory_pool, m_phmcrdxln, m_phmcrdxlnIndexLookup, pcr);
#ifdef 	GPOS_DEBUG
			BOOL fInserted =
#endif // GPOS_DEBUG
			m_phmcrdxlnIndexLookup->Insert(pcr, pdxln);
			GPOS_ASSERT(fInserted);
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnNLJoin
//
//	@doc:
//		Create a DXL nested loop join node from an optimizer nested loop
//		join expression
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnNLJoin
	(
	CExpression *pexprInnerNLJ,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprInnerNLJ);
	GPOS_ASSERT(3 == pexprInnerNLJ->Arity());

	// extract components
	CPhysical *pop = CPhysical::PopConvert(pexprInnerNLJ->Pop());

	CExpression *pexprOuterChild = (*pexprInnerNLJ)[0];
	CExpression *pexprInnerChild = (*pexprInnerNLJ)[1];
	CExpression *pexprScalar = (*pexprInnerNLJ)[2];


#ifdef GPOS_DEBUG
	// get children's relational properties
	CDrvdPropRelational *pdprelOuter =
			CDrvdPropRelational::Pdprel(pexprOuterChild->Pdp(CDrvdProp::EptRelational));

	CDrvdPropRelational *pdprelInner =
			CDrvdPropRelational::Pdprel(pexprInnerChild->Pdp(CDrvdProp::EptRelational));

	GPOS_ASSERT_IMP(COperator::EopPhysicalInnerIndexNLJoin != pop->Eopid() &&
					COperator::EopPhysicalLeftOuterIndexNLJoin != pop->Eopid()
			, pdprelInner->PcrsOuter()->IsDisjoint(pdprelOuter->PcrsOutput()) &&
			"detected outer references in NL inner child");
#endif // GPOS_DEBUG

	EdxlJoinType join_type = EdxljtSentinel;
	BOOL fIndexNLJ = false;
	switch (pop->Eopid())
	{
		case COperator::EopPhysicalInnerNLJoin:
			join_type = EdxljtInner;
			break;

		case COperator::EopPhysicalInnerIndexNLJoin:
			join_type = EdxljtInner;
			fIndexNLJ = true;
			StoreIndexNLJOuterRefs(pop);
			break;

		case COperator::EopPhysicalLeftOuterIndexNLJoin:
			join_type = EdxljtLeft;
			fIndexNLJ = true;
			StoreIndexNLJOuterRefs(pop);
			break;

		case COperator::EopPhysicalLeftOuterNLJoin:
			join_type = EdxljtLeft;
			break;

		case COperator::EopPhysicalLeftSemiNLJoin:
			join_type = EdxljtIn;
			break;

		case COperator::EopPhysicalLeftAntiSemiNLJoin:
			join_type = EdxljtLeftAntiSemijoin;
			break;

		case COperator::EopPhysicalLeftAntiSemiNLJoinNotIn:
			join_type = EdxljtLeftAntiSemijoinNotIn;
			break;

		default:
			GPOS_ASSERT(!"Invalid join type");
	}

	// translate relational child expressions
	CDXLNode *pdxlnOuterChild = CreateDXLNode(pexprOuterChild, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	CDXLNode *pdxlnInnerChild = CreateDXLNode(pexprInnerChild, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	CDXLNode *pdxlnCond = PdxlnScalar(pexprScalar);

	CDXLNode *pdxlnJoinFilter = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarJoinFilter(m_memory_pool));
	if (NULL != pdxlnCond)
	{
		pdxlnJoinFilter->AddChild(pdxlnCond);
	}

	// construct a join node
	CDXLPhysicalNLJoin *pdxlopNLJ = GPOS_NEW(m_memory_pool) CDXLPhysicalNLJoin(m_memory_pool, join_type,fIndexNLJ);

	// construct projection list
	// compute required columns
	GPOS_ASSERT(NULL != pexprInnerNLJ->Prpp());
	CColRefSet *pcrsOutput = pexprInnerNLJ->Prpp()->PcrsRequired();

	CDXLNode *proj_list_dxlnode = PdxlnProjList(pcrsOutput, pdrgpcr);

	CDXLNode *pdxlnNLJ = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopNLJ);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprInnerNLJ);
	pdxlnNLJ->SetProperties(dxl_properties);

	// construct an empty plan filter
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL);

	// add children
	pdxlnNLJ->AddChild(proj_list_dxlnode);
	pdxlnNLJ->AddChild(filter_dxlnode);
	pdxlnNLJ->AddChild(pdxlnJoinFilter);
	pdxlnNLJ->AddChild(pdxlnOuterChild);
	pdxlnNLJ->AddChild(pdxlnInnerChild);

#ifdef GPOS_DEBUG
	pdxlopNLJ->AssertValid(pdxlnNLJ, false /* validate_children */);
#endif

	return pdxlnNLJ;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::EdxljtHashJoin
//
//	@doc:
//		Return hash join type
//---------------------------------------------------------------------------
EdxlJoinType
CTranslatorExprToDXL::EdxljtHashJoin
	(
	CPhysicalHashJoin *popHJ
	)
{
	GPOS_ASSERT(CUtils::FHashJoin(popHJ));

	switch (popHJ->Eopid())
	{
		case COperator::EopPhysicalInnerHashJoin:
			return EdxljtInner;

		case COperator::EopPhysicalLeftOuterHashJoin:
			return EdxljtLeft;

		case COperator::EopPhysicalLeftSemiHashJoin:
			return EdxljtIn;

		case COperator::EopPhysicalLeftAntiSemiHashJoin:
			return EdxljtLeftAntiSemijoin;

		case COperator::EopPhysicalLeftAntiSemiHashJoinNotIn:
			return EdxljtLeftAntiSemijoinNotIn;

		default:
			GPOS_ASSERT(!"Invalid join type");
			return EdxljtSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnHashJoin
//
//	@doc:
//		Create a DXL hash join node from an optimizer hash join expression.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnHashJoin
	(
	CExpression *pexprHJ,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprHJ);

	GPOS_ASSERT(3 == pexprHJ->Arity());

	// extract components
	CPhysicalHashJoin *popHJ = CPhysicalHashJoin::PopConvert(pexprHJ->Pop());
	CExpression *pexprOuterChild = (*pexprHJ)[0];
	CExpression *pexprInnerChild = (*pexprHJ)[1];
	CExpression *pexprScalar = (*pexprHJ)[2];

	EdxlJoinType join_type = EdxljtHashJoin(popHJ);
	GPOS_ASSERT(popHJ->PdrgpexprOuterKeys()->Size() == popHJ->PdrgpexprInnerKeys()->Size());

	// translate relational child expression
	CDXLNode *pdxlnOuterChild = CreateDXLNode(pexprOuterChild, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	CDXLNode *pdxlnInnerChild = CreateDXLNode(pexprInnerChild, NULL /*pdrgpcr*/, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// construct hash condition
	CDXLNode *pdxlnHashCondList = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarHashCondList(m_memory_pool));

	// output of outer side
	CColRefSet *pcrsOuter =  CDrvdPropRelational::Pdprel(pexprOuterChild->Pdp(CDrvdProp::EptRelational))->PcrsOutput();

#ifdef GPOS_DEBUG
	// output of inner side
	CColRefSet *pcrsInner = CDrvdPropRelational::Pdprel(pexprInnerChild->Pdp(CDrvdProp::EptRelational))->PcrsOutput();
	ULONG ulHashJoinPreds = 0;
#endif // GPOS_DEBUG

	DrgPexpr *pdrgpexprPredicates = CPredicateUtils::PdrgpexprConjuncts(m_memory_pool, pexprScalar);
	DrgPexpr *pdrgpexprRemainingPredicates = GPOS_NEW(m_memory_pool) DrgPexpr(m_memory_pool);
	const ULONG ulSize = pdrgpexprPredicates->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CExpression *pexprPred = (*pdrgpexprPredicates)[ul];
		if (CPhysicalJoin::FHashJoinCompatible(pexprPred, pexprOuterChild, pexprInnerChild))
		{
			 CExpression *pexprPredOuter = NULL;
			 CExpression *pexprPredInner = NULL;
			 CPhysicalJoin::ExtractHashJoinExpressions(pexprPred, &pexprPredOuter, &pexprPredInner);

			 // align extracted columns with outer and inner children of the join
			 CColRefSet *pcrsPredInner = CDrvdPropScalar::Pdpscalar(pexprPredInner->PdpDerive())->PcrsUsed();
#ifdef GPOS_DEBUG
			 CColRefSet *pcrsPredOuter = CDrvdPropScalar::Pdpscalar(pexprPredOuter->PdpDerive())->PcrsUsed();
#endif // GPOS_DEBUG
			 if (pcrsOuter->ContainsAll(pcrsPredInner))
			 {
				 // extracted expressions are not aligned with join children, we need to swap them
				 GPOS_ASSERT(pcrsInner->ContainsAll(pcrsPredOuter));
				 std::swap(pexprPredOuter, pexprPredInner);
#ifdef GPOS_DEBUG
				 std::swap(pcrsPredOuter, pcrsPredInner);
#endif
			 }
			 GPOS_ASSERT(pcrsOuter->ContainsAll(pcrsPredOuter) && pcrsInner->ContainsAll(pcrsPredInner) &&
					 "hash join keys are not aligned with hash join children");

			 pexprPredOuter->AddRef();
			 pexprPredInner->AddRef();
			 // create hash join predicate based on conjunct type
			 if (CPredicateUtils::FEquality(pexprPred))
			 {
				pexprPred = CUtils::PexprScalarEqCmp(m_memory_pool, pexprPredOuter, pexprPredInner);
			 }
			 else
			 {
				GPOS_ASSERT(CPredicateUtils::FINDF(pexprPred));
				pexprPred = CUtils::PexprINDF(m_memory_pool, pexprPredOuter, pexprPredInner);
			 }

			 CDXLNode *pdxlnPred = PdxlnScalar(pexprPred);
			 pdxlnHashCondList->AddChild(pdxlnPred);
			 pexprPred->Release();
#ifdef GPOS_DEBUG
			 ulHashJoinPreds ++;
#endif // GPOS_DEBUG
		}
		else
		{
			pexprPred->AddRef();
			pdrgpexprRemainingPredicates->Append(pexprPred);
		}
	}
	GPOS_ASSERT(popHJ->PdrgpexprOuterKeys()->Size() == ulHashJoinPreds);

	CDXLNode *pdxlnJoinFilter = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarJoinFilter(m_memory_pool));
	if (0 < pdrgpexprRemainingPredicates->Size())
	{
		CExpression *pexprJoinCond = CPredicateUtils::PexprConjunction(m_memory_pool, pdrgpexprRemainingPredicates);
		CDXLNode *pdxlnJoinCond = PdxlnScalar(pexprJoinCond);
		pdxlnJoinFilter->AddChild(pdxlnJoinCond);
		pexprJoinCond->Release();
	}
	else
	{
		pdrgpexprRemainingPredicates->Release();
	}

	// construct a hash join node
	CDXLPhysicalHashJoin *pdxlopHJ = GPOS_NEW(m_memory_pool) CDXLPhysicalHashJoin(m_memory_pool, join_type);

	// construct projection list from required columns
	GPOS_ASSERT(NULL != pexprHJ->Prpp());
	CColRefSet *pcrsOutput = pexprHJ->Prpp()->PcrsRequired();
	CDXLNode *proj_list_dxlnode = PdxlnProjList(pcrsOutput, pdrgpcr);

	CDXLNode *pdxlnHJ = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopHJ);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprHJ);
	pdxlnHJ->SetProperties(dxl_properties);

	// construct an empty plan filter
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL);

	// add children
	pdxlnHJ->AddChild(proj_list_dxlnode);
	pdxlnHJ->AddChild(filter_dxlnode);
	pdxlnHJ->AddChild(pdxlnJoinFilter);
	pdxlnHJ->AddChild(pdxlnHashCondList);
	pdxlnHJ->AddChild(pdxlnOuterChild);
	pdxlnHJ->AddChild(pdxlnInnerChild);

	// cleanup 
	pdrgpexprPredicates->Release();

#ifdef GPOS_DEBUG
	pdxlopHJ->AssertValid(pdxlnHJ, false /* validate_children */);
#endif

	return pdxlnHJ;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::CheckValidity
//
//	@doc:
//		Check if the motion node is valid
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::CheckValidity
	(
	CDXLPhysicalMotion *pdxlopMotion
	)

{
	// validate the input segment info for Gather Motion
	// if Gather has only 1 segment when there are more hosts
	// it's obviously invalid and we fall back
	if (EdxlopPhysicalMotionGather == pdxlopMotion->GetDXLOperator())
	{
		if (m_pdrgpiSegments->Size() != pdxlopMotion->GetInputSegIdsArray()->Size())
		{
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiExpr2DXLUnsupportedFeature, GPOS_WSZ_LIT("GatherMotion input segments number does not match with the number of segments in the system"));
		}
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnMotion
//
//	@doc:
//		Create a DXL motion node from an optimizer motion expression
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnMotion
	(
	CExpression *pexprMotion,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprMotion);
	GPOS_ASSERT(1 == pexprMotion->Arity());

	// extract components
	CExpression *pexprChild = (*pexprMotion)[0];

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot*/);

	// construct a motion node
	CDXLPhysicalMotion *pdxlopMotion = NULL;
	BOOL fDuplicateHazardMotion = CUtils::FDuplicateHazardMotion(pexprMotion);
	switch (pexprMotion->Pop()->Eopid())
	{
		case COperator::EopPhysicalMotionGather:
			pdxlopMotion = GPOS_NEW(m_memory_pool) CDXLPhysicalGatherMotion(m_memory_pool);
			break;

		case COperator::EopPhysicalMotionBroadcast:
			pdxlopMotion = GPOS_NEW(m_memory_pool) CDXLPhysicalBroadcastMotion(m_memory_pool);
			break;

		case COperator::EopPhysicalMotionHashDistribute:
			pdxlopMotion = GPOS_NEW(m_memory_pool) CDXLPhysicalRedistributeMotion(m_memory_pool, fDuplicateHazardMotion);
			break;

		case COperator::EopPhysicalMotionRandom:
			pdxlopMotion = GPOS_NEW(m_memory_pool) CDXLPhysicalRandomMotion(m_memory_pool, fDuplicateHazardMotion);
			break;

		case COperator::EopPhysicalMotionRoutedDistribute:
			{
				CPhysicalMotionRoutedDistribute *popMotion =
						CPhysicalMotionRoutedDistribute::PopConvert(pexprMotion->Pop());
				CColRef *pcrSegmentId = dynamic_cast<const CDistributionSpecRouted* >(popMotion->Pds())->Pcr();

				pdxlopMotion = GPOS_NEW(m_memory_pool) CDXLPhysicalRoutedDistributeMotion(m_memory_pool, pcrSegmentId->UlId());
				break;
			}
		default:
			GPOS_ASSERT(!"Unrecognized motion type");
	}

	if (COperator::EopPhysicalMotionGather != pexprMotion->Pop()->Eopid())
	{
		(*pulNonGatherMotions)++;
	}

	GPOS_ASSERT(NULL != pdxlopMotion);

	// construct project list from child project list
	GPOS_ASSERT(NULL != child_dxlnode && 1 <= child_dxlnode->Arity());
	CDXLNode *pdxlnProjListChild = (*child_dxlnode)[0];

	CDXLNode *proj_list_dxlnode = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// set input and output segment information
	pdxlopMotion->SetSegmentInfo(GetInputSegIdsArray(pexprMotion), GetOutputSegIdsArray(pexprMotion));

	CheckValidity(pdxlopMotion);

	CDXLNode *pdxlnMotion = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopMotion);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprMotion);
	pdxlnMotion->SetProperties(dxl_properties);

	// construct an empty filter node
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL /*pdxlnCond*/);

	// construct sort column list
	CDXLNode *sort_col_list_dxl = GetSortColListDXL(pexprMotion);

	// add children
	pdxlnMotion->AddChild(proj_list_dxlnode);
	pdxlnMotion->AddChild(filter_dxlnode);
	pdxlnMotion->AddChild(sort_col_list_dxl);

	if (COperator::EopPhysicalMotionHashDistribute == pexprMotion->Pop()->Eopid())
	{
		// construct a hash expr list node
		CPhysicalMotionHashDistribute *popHashDistribute = CPhysicalMotionHashDistribute::PopConvert(pexprMotion->Pop());
		CDistributionSpecHashed *pdsHashed = CDistributionSpecHashed::PdsConvert(popHashDistribute->Pds());
		CDXLNode *pdxlnHashExprList = PdxlnHashExprList(pdsHashed->Pdrgpexpr());
		pdxlnMotion->AddChild(pdxlnHashExprList);
	}

	pdxlnMotion->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopMotion->AssertValid(pdxlnMotion, false /* validate_children */);
#endif

	return pdxlnMotion;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnMaterialize
//
//	@doc:
//		Create a DXL materialize node from an optimizer spool expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnMaterialize
	(
	CExpression *pexprSpool,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprSpool);

	GPOS_ASSERT(1 == pexprSpool->Arity());

	// extract components
	CExpression *pexprChild = (*pexprSpool)[0];

	// translate relational child expression
	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// construct a materialize node
	CDXLPhysicalMaterialize *pdxlopMat = GPOS_NEW(m_memory_pool) CDXLPhysicalMaterialize(m_memory_pool, true /* fEager */);

	// construct project list from child project list
	GPOS_ASSERT(NULL != child_dxlnode && 1 <= child_dxlnode->Arity());
	CDXLNode *pdxlnProjListChild = (*child_dxlnode)[0];
	CDXLNode *proj_list_dxlnode = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	CDXLNode *pdxlnMaterialize = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopMat);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprSpool);
	pdxlnMaterialize->SetProperties(dxl_properties);

	// construct an empty filter node
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL /* pdxlnCond */);

	// add children
	pdxlnMaterialize->AddChild(proj_list_dxlnode);
	pdxlnMaterialize->AddChild(filter_dxlnode);
	pdxlnMaterialize->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopMat->AssertValid(pdxlnMaterialize, false /* validate_children */);
#endif

	return pdxlnMaterialize;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSequence
//
//	@doc:
//		Create a DXL sequence node from an optimizer sequence expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnSequence
	(
	CExpression *pexprSequence,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprSequence);

	const ULONG arity = pexprSequence->Arity();
	GPOS_ASSERT(0 < arity);

	// construct sequence node
	CDXLPhysicalSequence *pdxlopSequence = GPOS_NEW(m_memory_pool) CDXLPhysicalSequence(m_memory_pool);
	CDXLNode *pdxlnSequence = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopSequence);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprSequence);
	pdxlnSequence->SetProperties(dxl_properties);

	// translate children
	DXLNodeArray *pdrgpdxlnChildren = GPOS_NEW(m_memory_pool) DXLNodeArray(m_memory_pool);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprChild = (*pexprSequence)[ul];

		DrgPcr *pdrgpcrChildOutput = NULL;
		if (ul == arity - 1)
		{
			// impose output columns on last child
			pdrgpcrChildOutput = pdrgpcr;
		}

		CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcrChildOutput, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
		pdrgpdxlnChildren->Append(child_dxlnode);
	}

	// construct project list from the project list of the last child
	CDXLNode *pdxlnLastChild = (*pdrgpdxlnChildren)[arity - 1];
	CDXLNode *pdxlnProjListChild = (*pdxlnLastChild)[0];

	CDXLNode *proj_list_dxlnode = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnProjListChild);
	pdxlnSequence->AddChild(proj_list_dxlnode);

	// add children
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CDXLNode *pdxlnChid = (*pdrgpdxlnChildren)[ul];
		pdxlnChid->AddRef();
		pdxlnSequence->AddChild(pdxlnChid);
	}

	pdrgpdxlnChildren->Release();

#ifdef GPOS_DEBUG
	pdxlopSequence->AssertValid(pdxlnSequence, false /* validate_children */);
#endif

	return pdxlnSequence;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelector
//
//	@doc:
//		Translate a partition selector into DXL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelector
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	return PdxlnPartitionSelector(pexpr, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, NULL /*pexprScalarCond*/, NULL /*dxl_properties*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelector
//
//	@doc:
//		Translate a partition selector into DXL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelector
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CExpression *pexprScalarCond,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexpr->Pop());

	CExpression *pexprScalar = popSelector->PexprCombinedPred();
	if (CUtils::FScalarConstTrue(pexprScalar))
	{
		return PdxlnPartitionSelectorExpand(pexpr, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, pexprScalarCond, dxl_properties);
	}

	return PdxlnPartitionSelectorFilter(pexpr, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, pexprScalarCond, dxl_properties);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelectorDML
//
//	@doc:
//		Translate a DML partition selector into DXL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelectorDML
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	CExpression *pexprChild = (*pexpr)[0];
	CPhysicalPartitionSelectorDML *popSelector = CPhysicalPartitionSelectorDML::PopConvert(pexpr->Pop());

	// translate child
	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	// construct project list
	IMDId *pmdid = popSelector->MDId();
	GPOS_ASSERT(1 <= child_dxlnode->Arity());
	CDXLNode *pdxlnPrL = CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector
							(
							m_memory_pool,
							m_pmda,
							m_pcf,
							m_phmcrdxln,
							true, //fUseChildProjList
							(*child_dxlnode)[0],
							popSelector->PcrOid(),
							popSelector->UlPartLevels(),
							CUtils::FGeneratePartOid(pmdid)
							);

	// translate filters
	CDXLNode *pdxlnEqFilters = NULL;
	CDXLNode *pdxlnFilters = NULL;
	CDXLNode *pdxlnResidual = NULL;
	TranslatePartitionFilters(pexpr, true /*fPassThrough*/, &pdxlnEqFilters, &pdxlnFilters, &pdxlnResidual);

	// since there is no propagation for DML, we create a const null expression
	const IMDTypeInt4 *pmdtypeint4 = m_pmda->PtMDType<IMDTypeInt4>();
	CDXLDatum *pdxldatumNull = pmdtypeint4->PdxldatumNull(m_memory_pool);
	CDXLNode *pdxlnPropagation = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarConstValue(m_memory_pool, pdxldatumNull));

	// true printable filter
	CDXLNode *pdxlnPrintable = CTranslatorExprToDXLUtils::PdxlnBoolConst(m_memory_pool, m_pmda, true /*value*/);

	// construct PartitionSelector node
	IMDId *pmdidRel = popSelector->MDId();
	pmdidRel->AddRef();

	CDXLNode *pdxlnSelector = CTranslatorExprToDXLUtils::PdxlnPartitionSelector
									(
									m_memory_pool,
									pmdidRel,
									popSelector->UlPartLevels(),
									0, // scan_id
									GetProperties(pexpr),
									pdxlnPrL,
									pdxlnEqFilters,
									pdxlnFilters,
									pdxlnResidual,
									pdxlnPropagation,
									pdxlnPrintable,
									child_dxlnode
									);

#ifdef GPOS_DEBUG
	pdxlnSelector->GetOperator()->AssertValid(pdxlnSelector, false /* validate_children */);
#endif

	return pdxlnSelector;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartFilterList
//
//	@doc:
//		Return a DXL part filter list. Can be used for the equality filters or
//		the general filters
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartFilterList
	(
	CExpression *pexpr,
	BOOL fEqFilters
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexpr->Pop());

	CDXLNode *pdxlnFilters = NULL;
	if (fEqFilters)
	{
		pdxlnFilters = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOpList(m_memory_pool, CDXLScalarOpList::EdxloplistEqFilterList));
	}
	else
	{
		pdxlnFilters = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOpList(m_memory_pool, CDXLScalarOpList::EdxloplistFilterList));
	}

	const ULONG ulPartLevels = popSelector->UlPartLevels();
	GPOS_ASSERT(1 <= ulPartLevels);

	for (ULONG ul = 0; ul < ulPartLevels; ul++)
	{
		CExpression *pexprFilter = NULL;
		if (fEqFilters)
		{
			pexprFilter = popSelector->PexprEqFilter(ul);
		}
		else
		{
			pexprFilter = popSelector->PexprFilter(ul);
		}

		CDXLNode *filter_dxlnode = NULL;
		if (NULL == pexprFilter)
		{
			filter_dxlnode = CTranslatorExprToDXLUtils::PdxlnBoolConst(m_memory_pool, m_pmda, true /*value*/);
		}
		else
		{
			filter_dxlnode = PdxlnScalar(pexprFilter);
		}
		pdxlnFilters->AddChild(filter_dxlnode);
	}

	return pdxlnFilters;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelectorExpand
//
//	@doc:
//		Translate an expand-based partition resolver into DXL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelectorExpand
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CExpression *pexprScalarCond,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	CExpression *pexprChild = (*pexpr)[0];

	GPOS_ASSERT_IMP
		(
		NULL != pexprScalarCond,
		(COperator::EopPhysicalDynamicTableScan == pexprChild->Pop()->Eopid() ||
		 COperator::EopPhysicalDynamicIndexScan == pexprChild->Pop()->Eopid() ||
		 COperator::EopPhysicalDynamicBitmapTableScan == pexprChild->Pop()->Eopid())
		&& "Inlining predicates only allowed in DynamicTableScan, DynamicIndexScan and DynamicBitmapTableScan"
		);

	CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexpr->Pop());
	const ULONG ulLevels = popSelector->UlPartLevels();

	// translate child
	CDXLNode *child_dxlnode = PdxlnPartitionSelectorChild(pexprChild, pexprScalarCond, dxl_properties, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);

	// project list
	IMDId *pmdid = popSelector->MDId();
	const IMDRelation *pmdrel = (IMDRelation *) m_pmda->Pmdrel(pmdid);
	CDXLNode *pdxlnPrL = CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector
							(
							m_memory_pool,
							m_pmda,
							m_pcf,
							m_phmcrdxln,
							false, //fUseChildProjList
							NULL, //pdxlnPrLchild
							NULL, //pcrOid
							ulLevels,
							CUtils::FGeneratePartOid(pmdid)
							);

	// translate filters
	CDXLNode *pdxlnEqFilters = NULL;
	CDXLNode *pdxlnFilters = NULL;
	CDXLNode *pdxlnResidual = NULL;
	TranslatePartitionFilters(pexpr, true /*fPassThrough*/, &pdxlnEqFilters, &pdxlnFilters, &pdxlnResidual);

	// construct propagation expression
	CPartIndexMap *ppimDrvd = m_pdpplan->Ppim();
	ULONG scan_id = popSelector->UlScanId();
	CDXLNode *pdxlnPropagation = CTranslatorExprToDXLUtils::PdxlnPropExprPartitionSelector
									(
									m_memory_pool,
									m_pmda,
									m_pcf,
									ppimDrvd->FPartialScans(scan_id),
									ppimDrvd->Ppartcnstrmap(scan_id),
									popSelector->Pdrgpdrgpcr(),
									scan_id,
									pmdrel->PdrgpszPartTypes()
									);

	// translate printable filter
	CExpression *pexprPrintable = popSelector->PexprCombinedPred();
	GPOS_ASSERT(NULL != pexprPrintable);
	CDXLNode *pdxlnPrintable = PdxlnScalar(pexprPrintable);

	// construct PartitionSelector node
	IMDId *pmdidRel = popSelector->MDId();
	pmdidRel->AddRef();

	CDXLNode *pdxlnSelector = CTranslatorExprToDXLUtils::PdxlnPartitionSelector
									(
									m_memory_pool,
									pmdidRel,
									ulLevels,
									scan_id,
									CTranslatorExprToDXLUtils::GetProperties(m_memory_pool),
									pdxlnPrL,
									pdxlnEqFilters,
									pdxlnFilters,
									pdxlnResidual,
									pdxlnPropagation,
									pdxlnPrintable
									);

	// construct sequence node
	CDXLPhysicalSequence *pdxlopSequence = GPOS_NEW(m_memory_pool) CDXLPhysicalSequence(m_memory_pool);
	CDXLNode *pdxlnSequence = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopSequence);

	CDXLPhysicalProperties *pdxlpropSeq = CTranslatorExprToDXLUtils::PdxlpropCopy(m_memory_pool, child_dxlnode);
	pdxlnSequence->SetProperties(pdxlpropSeq);

	// construct sequence's project list from the project list of the last child
	CDXLNode *pdxlnPrLChild = (*child_dxlnode)[0];
	CDXLNode *pdxlnPrLSequence = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnPrLChild);

	pdxlnSequence->AddChild(pdxlnPrLSequence);
	pdxlnSequence->AddChild(pdxlnSelector);
	pdxlnSequence->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopSequence->AssertValid(pdxlnSequence, false /* validate_children */);
#endif

	return pdxlnSequence;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelectorFilter
//
//	@doc:
//		Translate a filter-based partition selector into DXL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelectorFilter
	(
	CExpression *pexpr,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML,
	CExpression *pexprScalarCond,
	CDXLPhysicalProperties *dxl_properties
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexpr->Pop());
	CPartIndexMap *ppimDrvd = m_pdpplan->Ppim();
	ULONG scan_id = popSelector->UlScanId();
	ULONG ulLevels = popSelector->UlPartLevels();
	BOOL fPartialScans = ppimDrvd->FPartialScans(scan_id);
	PartCnstrMap *ppartcnstrmap = ppimDrvd->Ppartcnstrmap(scan_id);

	BOOL fPassThrough = FEqPartFiltersAllLevels(pexpr, false /*fCheckGeneralFilters*/) && !fPartialScans;
#ifdef GPOS_DEBUG
	BOOL fPoint = FEqPartFiltersAllLevels(pexpr, true /*fCheckGeneralFilters*/) && !fPartialScans;
	GPOS_ASSERT_IMP(!fPoint && fPartialScans, NULL != ppartcnstrmap);
#endif

	CExpression *pexprChild = (*pexpr)[0];

	GPOS_ASSERT_IMP
		(
		NULL != pexprScalarCond,
		(COperator::EopPhysicalDynamicTableScan == pexprChild->Pop()->Eopid() ||
		 COperator::EopPhysicalDynamicIndexScan == pexprChild->Pop()->Eopid() ||
		 COperator::EopPhysicalDynamicBitmapTableScan == pexprChild->Pop()->Eopid())
		&& "Inlining predicates only allowed in DynamicTableScan, DynamicIndexScan and DynamicBitmapTableScan"
		);

	// translate child
	CDXLNode *child_dxlnode = PdxlnPartitionSelectorChild(pexprChild, pexprScalarCond, dxl_properties, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	CDXLNode *pdxlnPrLChild = (*child_dxlnode)[0];

	CDrvdPropRelational *pdprel = CDrvdPropRelational::Pdprel(pexprChild->Pdp(CDrvdProp::EptRelational));

	// we add a sequence if the scan id is found below the resolver
	BOOL fNeedSequence = pdprel->Ppartinfo()->FContainsScanId(popSelector->UlScanId());

	// project list
	IMDId *pmdid = popSelector->MDId();
	const IMDRelation *pmdrel = (IMDRelation *) m_pmda->Pmdrel(pmdid);
	CDXLNode *pdxlnPrL = CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector
							(
							m_memory_pool,
							m_pmda,
							m_pcf,
							m_phmcrdxln,
							!fNeedSequence,
							pdxlnPrLChild,
							NULL /*pcrOid*/,
							ulLevels,
							CUtils::FGeneratePartOid(pmdid)
							);

	// translate filters
	CDXLNode *pdxlnEqFilters = NULL;
	CDXLNode *pdxlnFilters = NULL;
	CDXLNode *pdxlnResidual = NULL;
	TranslatePartitionFilters(pexpr, fPassThrough, &pdxlnEqFilters, &pdxlnFilters, &pdxlnResidual);

	// construct propagation expression
	CDXLNode *pdxlnPropagation = CTranslatorExprToDXLUtils::PdxlnPropExprPartitionSelector
									(
									m_memory_pool,
									m_pmda,
									m_pcf,
									!fPassThrough && fPartialScans, //fConditional
									ppartcnstrmap,
									popSelector->Pdrgpdrgpcr(),
									popSelector->UlScanId(),
									pmdrel->PdrgpszPartTypes()
									);

	// translate printable filter
	CExpression *pexprPrintable = popSelector->PexprCombinedPred();
	GPOS_ASSERT(NULL != pexprPrintable);
	CDXLNode *pdxlnPrintable = PdxlnScalar(pexprPrintable);

	// construct PartitionSelector node
	IMDId *pmdidRel = popSelector->MDId();
	pmdidRel->AddRef();

	CDXLNode *pdxlnSelectorChild = NULL;
	if (!fNeedSequence)
	{
		pdxlnSelectorChild = child_dxlnode;
	}

	CDXLNode *pdxlnSelector = CTranslatorExprToDXLUtils::PdxlnPartitionSelector
									(
									m_memory_pool,
									pmdidRel,
									ulLevels,
									scan_id,
									CTranslatorExprToDXLUtils::GetProperties(m_memory_pool),
									pdxlnPrL,
									pdxlnEqFilters,
									pdxlnFilters,
									pdxlnResidual,
									pdxlnPropagation,
									pdxlnPrintable,
									pdxlnSelectorChild
									);

	CDXLNode *pdxlnReturned = pdxlnSelector;
	if (fNeedSequence)
	{
		CDXLPhysicalSequence *pdxlopSequence = GPOS_NEW(m_memory_pool) CDXLPhysicalSequence(m_memory_pool);
		CDXLNode *pdxlnSequence = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopSequence);
		CDXLPhysicalProperties *pdxlpropSeq = CTranslatorExprToDXLUtils::PdxlpropCopy(m_memory_pool, child_dxlnode);
		pdxlnSequence->SetProperties(pdxlpropSeq);

		// construct sequence's project list from the project list of the last child
		CDXLNode *pdxlnPrLSequence = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnPrLChild);

		pdxlnSequence->AddChild(pdxlnPrLSequence);
		pdxlnSequence->AddChild(pdxlnSelector);
		pdxlnSequence->AddChild(child_dxlnode);

		pdxlnReturned = pdxlnSequence;
	}

#ifdef GPOS_DEBUG
	pdxlnReturned->GetOperator()->AssertValid(pdxlnReturned, false /* validate_children */);
#endif

	return pdxlnReturned;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::FEqPartFiltersAllLevels
//
//	@doc:
//		Check whether the given partition selector only has equality filters
//		or no filters on all partitioning levels. Return false if it has
//		non-equality filters. If fCheckGeneralFilters is true then the function
//		checks whether the content of general filter is conjunction of equality
//		filter or not. If it is false, we always view the general filter as
//		non-equality filter if the pexprFilter is not null.
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXL::FEqPartFiltersAllLevels
	(
	CExpression *pexpr,
	BOOL fCheckGeneralFilters
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexpr->Pop());
	const ULONG ulPartLevels = popSelector->UlPartLevels();
	GPOS_ASSERT(1 <= ulPartLevels);

	for (ULONG ul = 0; ul < ulPartLevels; ul++)
	{
		CExpression *pexprEqFilter = popSelector->PexprEqFilter(ul);
		CExpression *pexprFilter = popSelector->PexprFilter(ul);

		if (NULL == pexprEqFilter && NULL != pexprFilter)
		{
			if (!fCheckGeneralFilters || !CPredicateUtils::FConjunctionOfEqComparisons(m_memory_pool, pexprFilter))
			{
				return false;
			}
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::TranslatePartitionFilters
//
//	@doc:
//		Translate partition selector filters
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::TranslatePartitionFilters
	(
	CExpression *pexprPartSelector,
	BOOL fPassThrough,
	CDXLNode **ppdxlnEqFilters,		// output: translated equality filters
	CDXLNode **ppdxlnFilters,		// output: translated non-equality filters
	CDXLNode **ppdxlnResidual		// output: translated residual filters
	)
{
	GPOS_ASSERT(NULL != pexprPartSelector);

	if (fPassThrough)
	{
		*ppdxlnEqFilters = PdxlnPartFilterList(pexprPartSelector, true /*fEqFilters*/);
		*ppdxlnFilters = PdxlnPartFilterList(pexprPartSelector, false /*fEqFilters*/);

		CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexprPartSelector->Pop());
		CExpression *pexprResidual = popSelector->PexprResidualPred();
		if (NULL == pexprResidual)
		{
			*ppdxlnResidual = CTranslatorExprToDXLUtils::PdxlnBoolConst(m_memory_pool, m_pmda, true /*value*/);
		}
		else
		{
			*ppdxlnResidual = PdxlnScalar(pexprResidual);
		}

		return;
	}

	ConstructLevelFilters4PartitionSelector(pexprPartSelector, ppdxlnEqFilters, ppdxlnFilters);

	// TODO:  - Apr 11, 2014; translate the residual filter. Take into account
	// that this might be an arbitrary scalar expression on multiple part keys. Right
	// now we assume no residual filter in this case
	*ppdxlnResidual = CTranslatorExprToDXLUtils::PdxlnBoolConst(m_memory_pool, m_pmda, true /*value*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::ConstructLevelFilters4PartitionSelector
//
//	@doc:
// 		Construct the level filter lists for partition selector
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::ConstructLevelFilters4PartitionSelector
	(
	CExpression *pexprPartSelector,
	CDXLNode **ppdxlnEqFilters,
	CDXLNode **ppdxlnFilters
	)
{
	GPOS_ASSERT(NULL != pexprPartSelector);
	CPhysicalPartitionSelector *popSelector = CPhysicalPartitionSelector::PopConvert(pexprPartSelector->Pop());
	const IMDRelation *pmdrel = (IMDRelation *) m_pmda->Pmdrel(popSelector->MDId());

	const ULONG ulPartLevels = popSelector->UlPartLevels();
	GPOS_ASSERT(1 <= ulPartLevels);

	DrgDrgPcr *pdrgpdrgpcrPartKeys = popSelector->Pdrgpdrgpcr();
	CBitSet *pbsDefaultParts = NULL;
	IMDPartConstraint *pmdpartcnstr = m_pmda->Pmdrel(popSelector->MDId())->Pmdpartcnstr();
	if (NULL != pmdpartcnstr)
		pbsDefaultParts = CUtils::Pbs(m_memory_pool, pmdpartcnstr->PdrgpulDefaultParts());

	*ppdxlnFilters = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOpList(m_memory_pool, CDXLScalarOpList::EdxloplistFilterList));
	*ppdxlnEqFilters = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOpList(m_memory_pool, CDXLScalarOpList::EdxloplistEqFilterList));

	for (ULONG ulLevel = 0; ulLevel < ulPartLevels; ulLevel++)
	{
		CColRef *pcrPartKey = CUtils::PcrExtractPartKey(pdrgpdrgpcrPartKeys, ulLevel);
		IMDId *pmdidTypePartKey = pcrPartKey->Pmdtype()->MDId();
		CHAR szPartType = pmdrel->SzPartType(ulLevel);
		BOOL fRangePart = IMDRelation::ErelpartitionRange == szPartType;

		CDXLNode *filter_dxlnode = NULL;
		BOOL fDefaultPartition = pbsDefaultParts ? pbsDefaultParts->Get(ulLevel) : false;

		BOOL fLTComparison = false;
		BOOL fGTComparison = false;
		BOOL fEQComparison = false;

		// check if there is an equality filter on current level
		CExpression *pexprEqFilter = popSelector->PexprEqFilter(ulLevel);
		if (NULL != pexprEqFilter)
		{
			CDXLNode *pdxlnEq = PdxlnScalar(pexprEqFilter);
			IMDId *pmdidTypeOther = CScalar::PopConvert(pexprEqFilter->Pop())->MDIdType();
			fEQComparison = true;

			if (fRangePart)
			{
				filter_dxlnode = CTranslatorExprToDXLUtils::PdxlnRangeFilterEqCmp
								(
								m_memory_pool,
								m_pmda,
								pdxlnEq,
								pmdidTypePartKey,
								pmdidTypeOther,
								NULL /*pmdidTypeCastExpr*/,
								NULL /*pmdidCastFunc*/,
								ulLevel
								);
			}
			else // list partition
			{
				// Create a ScalarIdent expression from the partition key
				CDXLNode *pdxlnPartKey = CTranslatorExprToDXLUtils::PdxlnListFilterPartKey
															(
															m_memory_pool,
															m_pmda,
															CUtils::PexprScalarIdent(m_memory_pool, pcrPartKey),
															pmdidTypePartKey,
															ulLevel
															);

				filter_dxlnode = CTranslatorExprToDXLUtils::PdxlnListFilterScCmp
								(
								m_memory_pool,
								m_pmda,
								pdxlnPartKey,
								pdxlnEq,
								pmdidTypePartKey,
								pmdidTypeOther,
								IMDType::EcmptEq,
								ulLevel,
								fDefaultPartition
								);
			}
		}

		// check general filters on current level
		CExpression *pexprFilter = popSelector->PexprFilter(ulLevel);
		if (NULL != pexprFilter)
		{
			DrgPexpr *pdrgpexprConjuncts = CPredicateUtils::PdrgpexprConjuncts(m_memory_pool, pexprFilter);
			const ULONG length = pdrgpexprConjuncts->Size();

			for (ULONG ul = 0; ul < length; ul++)
			{
				CDXLNode *pdxlnScCmp = PdxlnPredOnPartKey
										(
										(*pdrgpexprConjuncts)[ul],
										pcrPartKey,
										pmdidTypePartKey,
										ulLevel,
										fRangePart,
										&fLTComparison,
										&fGTComparison,
										&fEQComparison
										);

				filter_dxlnode = CTranslatorExprToDXLUtils::PdxlnCombineBoolean(m_memory_pool, filter_dxlnode, pdxlnScCmp, Edxland);
			}

			pdrgpexprConjuncts->Release();
		}

		if (NULL != filter_dxlnode && fRangePart)
		{
			CDXLNode *pdxlnDefaultAndOpenEnded = CTranslatorExprToDXLUtils::PdxlnRangeFilterDefaultAndOpenEnded
										(
										m_memory_pool,
										ulLevel,
										fLTComparison,
										fGTComparison,
										fEQComparison,
										fDefaultPartition
										);

			filter_dxlnode = CTranslatorExprToDXLUtils::PdxlnCombineBoolean(m_memory_pool, filter_dxlnode, pdxlnDefaultAndOpenEnded, Edxlor);
		}

		if (NULL == filter_dxlnode)
		{
			filter_dxlnode = CTranslatorExprToDXLUtils::PdxlnBoolConst(m_memory_pool, m_pmda, true /*value*/);
		}

		(*ppdxlnFilters)->AddChild(filter_dxlnode);
		(*ppdxlnEqFilters)->AddChild(CTranslatorExprToDXLUtils::PdxlnBoolConst(m_memory_pool, m_pmda, true /*value*/));
	}

	if (NULL != pbsDefaultParts)
	{
		pbsDefaultParts->Release();
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPredOnPartKey
//
//	@doc:
// 		Translate a general predicate on a part key and update the various
//		comparison type flags accordingly
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPredOnPartKey
	(
	CExpression *pexprPred,
	CColRef *pcrPartKey,
	IMDId *pmdidTypePartKey,
	ULONG ulPartLevel,
	BOOL fRangePart,
	BOOL *pfLTComparison,	// input/output
	BOOL *pfGTComparison,	// input/output
	BOOL *pfEQComparison	// input/output
	)
{
	if (CPredicateUtils::FComparison(pexprPred))
	{
		return PdxlnScCmpPartKey(pexprPred, pcrPartKey, pmdidTypePartKey, ulPartLevel, fRangePart, pfLTComparison, pfGTComparison, pfEQComparison);
	}

	if (CUtils::FScalarNullTest(pexprPred))
	{
#ifdef GPOS_DEBUG
		CExpression *pexprChild = (*pexprPred)[0];
		GPOS_ASSERT(CUtils::FScalarIdent(pexprChild, pcrPartKey) || CCastUtils::FBinaryCoercibleCastedScId(pexprChild, pcrPartKey));
#endif //GPOS_DEBUG

		return PdxlnScNullTestPartKey(pmdidTypePartKey, ulPartLevel, fRangePart, true /*fIsNull*/);
	}

	if (CUtils::FScalarNotNull(pexprPred))
	{
#ifdef GPOS_DEBUG
		CExpression *pexprIsNull = (*pexprPred)[0];
		CExpression *pexprChild = (*pexprIsNull)[0];
		GPOS_ASSERT(CUtils::FScalarIdent(pexprChild, pcrPartKey) || CCastUtils::FBinaryCoercibleCastedScId(pexprChild, pcrPartKey));
#endif //GPOS_DEBUG

		*pfEQComparison = true;
		return PdxlnScNullTestPartKey(pmdidTypePartKey, ulPartLevel, fRangePart, false /*fIsNull*/);
	}

	if (CPredicateUtils::FCompareIdentToConstArray(pexprPred))
	{
		return PdxlArrayExprOnPartKey(pexprPred, pcrPartKey, pmdidTypePartKey, ulPartLevel, fRangePart, pfLTComparison, pfGTComparison, pfEQComparison);
	}

	GPOS_ASSERT(CPredicateUtils::FOr(pexprPred) || CPredicateUtils::FAnd(pexprPred));

	return PdxlnConjDisjOnPartKey(pexprPred, pcrPartKey, pmdidTypePartKey, ulPartLevel, fRangePart, pfLTComparison, pfGTComparison, pfEQComparison);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlArrayInOnPartKey
//
//	@doc:
//		Translates an array expression on a partition key to a disjunction because
//		the DXL partition translator requires expressions containing only LT, GT,
//		or EQ comparisons
//		For example the expression:
//			X IN (1,2,3) cannot be translated
//		but when converted into a constraint and then converted into a disjunction
//			X = 1 OR x = 2 OR x = 3
//		it can be converted to DXL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlArrayExprOnPartKey
	(
	CExpression *pexprPred,
	CColRef *pcrPartKey,
	IMDId *pmdidTypePartKey,
	ULONG ulPartLevel,
	BOOL fRangePart,
	BOOL *pfLTComparison,	// input/output
	BOOL *pfGTComparison,	// input/output
	BOOL *pfEQComparison	// input/output
	)
{
	GPOS_ASSERT(CUtils::FScalarArrayCmp(pexprPred));

	CConstraintInterval* pci = CConstraintInterval::PcnstrIntervalFromScalarArrayCmp(m_memory_pool, pexprPred, pcrPartKey);
	GPOS_ASSERT(NULL != pci);

	// convert the interval into a disjunction
	// (do not use CScalarArrayCmp::PexprExpand, it will use non-range
	// comparators which cannot be translated to a partition filter)
	CExpression *pexprDisj = pci->PexprConstructDisjunctionScalar(m_memory_pool);

	CDXLNode* pdxln = PdxlnConjDisjOnPartKey(pexprDisj, pcrPartKey, pmdidTypePartKey, ulPartLevel, fRangePart, pfLTComparison, pfGTComparison, pfEQComparison);
	pexprDisj->Release();
	pci->Release();


	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnConjDisjOnPartKey
//
//	@doc:
// 		Translate a conjunctive or disjunctive predicate on a part key and update
//		the various comparison type flags accordingly
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnConjDisjOnPartKey
	(
	CExpression *pexprPred,
	CColRef *pcrPartKey,
	IMDId *pmdidTypePartKey,
	ULONG ulPartLevel,
	BOOL fRangePart,
	BOOL *pfLTComparison,	// input/output
	BOOL *pfGTComparison,	// input/output
	BOOL *pfEQComparison	// input/output
	)
{
	GPOS_ASSERT(CPredicateUtils::FOr(pexprPred) || CPredicateUtils::FAnd(pexprPred));

	DrgPexpr *pdrgpexprChildren = NULL;
	EdxlBoolExprType edxlbet = Edxland;
	if (CPredicateUtils::FAnd(pexprPred))
	{
		pdrgpexprChildren = CPredicateUtils::PdrgpexprConjuncts(m_memory_pool, pexprPred);
	}
	else
	{
		pdrgpexprChildren = CPredicateUtils::PdrgpexprDisjuncts(m_memory_pool, pexprPred);
		edxlbet = Edxlor;
	}

	const ULONG ulChildren = pdrgpexprChildren->Size();

	CDXLNode *pdxlnPred = NULL;
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		CExpression *pexprChild = (*pdrgpexprChildren)[ul];
		CDXLNode *child_dxlnode = PdxlnPredOnPartKey(pexprChild, pcrPartKey, pmdidTypePartKey, ulPartLevel, fRangePart, pfLTComparison, pfGTComparison, pfEQComparison);

		if (NULL == pdxlnPred)
		{
			pdxlnPred = child_dxlnode;
		}
		else
		{
			pdxlnPred = CTranslatorExprToDXLUtils::PdxlnCombineBoolean(m_memory_pool, pdxlnPred, child_dxlnode, edxlbet);
		}
	}

	pdrgpexprChildren->Release();

	return pdxlnPred;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCmpPartKey
//
//	@doc:
// 		Translate a scalar comparison on a part key and update the various
//		comparison type flags accordingly
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCmpPartKey
	(
	CExpression *pexprScCmp,
	CColRef *pcrPartKey,
	IMDId *pmdidTypePartKey,
	ULONG ulPartLevel,
	BOOL fRangePart,
	BOOL *pfLTComparison,	// input/output
	BOOL *pfGTComparison,	// input/output
	BOOL *pfEQComparison	// input/output
	)
{
	GPOS_ASSERT(CPredicateUtils::FComparison(pexprScCmp));

	// extract components
	CExpression *pexprPartKey = NULL;
	CExpression *pexprOther = NULL;
	IMDType::ECmpType ecmpt = IMDType::EcmptOther;

	CPredicateUtils::ExtractComponents(pexprScCmp, pcrPartKey, &pexprPartKey, &pexprOther, &ecmpt);

	*pfLTComparison = *pfLTComparison || (IMDType::EcmptL == ecmpt) || (IMDType::EcmptLEq == ecmpt);
	*pfGTComparison = *pfGTComparison || (IMDType::EcmptG == ecmpt) || (IMDType::EcmptGEq == ecmpt);
	*pfEQComparison = *pfEQComparison || IMDType::EcmptEq == ecmpt;

	GPOS_ASSERT(NULL != pexprPartKey && NULL != pexprOther);
	GPOS_ASSERT(IMDType::EcmptOther != ecmpt);

	CDXLNode *pdxlnOther = PdxlnScalar(pexprOther);
	IMDId *pmdidTypeOther = CScalar::PopConvert(pexprOther->Pop())->MDIdType();
	IMDId *pmdidTypeCastExpr = NULL;
	IMDId *pmdidCastFunc = NULL;

	if (fRangePart) // range partition
	{
		CExpression *pexprNewPartKey = pexprPartKey;

		// If the pexprPartKey is not comparable with pexprOther, but can be casted to pexprOther,
		// and not yet casted, then we add a cast on top of pexprPartKey.
		if (!CMDAccessorUtils::FCmpExists(m_pmda, pmdidTypePartKey, pmdidTypeOther, ecmpt)
			&& CMDAccessorUtils::FCastExists(m_pmda, pmdidTypePartKey, pmdidTypeOther)
			&& COperator::EopScalarCast != pexprPartKey->Pop()->Eopid())
		{
			pexprNewPartKey = CUtils::PexprCast(m_memory_pool, m_pmda, pexprPartKey, pmdidTypeOther);
			pexprPartKey->Release();
		}

		CTranslatorExprToDXLUtils::ExtractCastMdids(pexprNewPartKey->Pop(), &pmdidTypeCastExpr, &pmdidCastFunc);

		return CTranslatorExprToDXLUtils::PdxlnRangeFilterScCmp
								(
								m_memory_pool,
								m_pmda,
								pdxlnOther,
								pmdidTypePartKey,
								pmdidTypeOther,
								pmdidTypeCastExpr,
								pmdidCastFunc,
								ecmpt,
								ulPartLevel
								);
	}
	else // list partition
	{
		ecmpt = CPredicateUtils::EcmptReverse(ecmpt);
		IMDId *pmdidTypePartKeyExpr = CScalar::PopConvert(pexprPartKey->Pop())->MDIdType();

		CDXLNode *pdxlnPartKeyExpr = CTranslatorExprToDXLUtils::PdxlnListFilterPartKey
																(
																m_memory_pool,
																m_pmda,
																pexprPartKey,
																pmdidTypePartKeyExpr,
																ulPartLevel
																);

		return CTranslatorExprToDXLUtils::PdxlnListFilterScCmp
								(
								m_memory_pool,
								m_pmda,
								pdxlnPartKeyExpr,
								pdxlnOther,
								pmdidTypePartKeyExpr,
								pmdidTypeOther,
								ecmpt,
								ulPartLevel,
								true
								);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScNullTestPartKey
//
//	@doc:
// 		Translate a scalar null test on a part key
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScNullTestPartKey
	(
	IMDId *pmdidTypePartKey,
	ULONG ulPartLevel,
	BOOL fRangePart,
	BOOL fIsNull
	)
{
	if (!fRangePart) // list partition
	{
		CDXLNode *pdxlnPartListNullTest = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarPartListNullTest(m_memory_pool, ulPartLevel, fIsNull));
		CDXLNode *pdxlnDefault = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarPartDefault(m_memory_pool, ulPartLevel));
		return GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBoolExpr(m_memory_pool, Edxlor), pdxlnPartListNullTest, pdxlnDefault);
	}

	pmdidTypePartKey->AddRef();
	CDXLNode *pdxlnPredicateMin = GPOS_NEW(m_memory_pool) CDXLNode
							(
							m_memory_pool,
							GPOS_NEW(m_memory_pool) CDXLScalarNullTest(m_memory_pool, fIsNull),
							GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarPartBound(m_memory_pool, ulPartLevel, pmdidTypePartKey, true /*fLower*/))
							);

	pmdidTypePartKey->AddRef();
	CDXLNode *pdxlnPredicateMax = GPOS_NEW(m_memory_pool) CDXLNode
							(
							m_memory_pool,
							GPOS_NEW(m_memory_pool) CDXLScalarNullTest(m_memory_pool, fIsNull),
							GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarPartBound(m_memory_pool, ulPartLevel, pmdidTypePartKey, false /*fLower*/))
							);

	// construct the conjunction of the predicate for the lower and upper bounds
	CDXLNode *pdxlnNullTests = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBoolExpr(m_memory_pool, Edxland), pdxlnPredicateMin, pdxlnPredicateMax);

	// AND that with the following: !(default || min_open || max_open)
	CDXLNode *pdxlnDefaultOrOpenEnded = CTranslatorExprToDXLUtils::PdxlnRangeFilterDefaultAndOpenEnded
								(
								m_memory_pool,
								ulPartLevel,
								true, //fLTComparison
								true, //fGTComparison
								false, //fEQComparison
								true //fDefaultPartition
								);

	CDXLNode *pdxlnNotDefaultOrOpenEnded = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBoolExpr(m_memory_pool, Edxlnot), pdxlnDefaultOrOpenEnded);

	return GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBoolExpr(m_memory_pool, Edxland), pdxlnNotDefaultOrOpenEnded, pdxlnNullTests);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnPartitionSelectorChild
//
//	@doc:
// 		Translate the child of a partition selector expression, pushing the given
//		scalar predicate if available
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnPartitionSelectorChild
	(
	CExpression *pexprChild,
	CExpression *pexprScalarCond,
	CDXLPhysicalProperties *dxl_properties,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT_IFF(NULL != pexprScalarCond, NULL != dxl_properties);

	if (NULL == pexprScalarCond)
	{
		return CreateDXLNode(pexprChild, pdrgpcr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true, false);
	}

	switch(pexprChild->Pop()->Eopid())
	{
		case COperator::EopPhysicalDynamicTableScan:
			return PdxlnDynamicTableScan(pexprChild, pdrgpcr, pdrgpdsBaseTables, pexprScalarCond, dxl_properties);
		case COperator::EopPhysicalDynamicIndexScan:
			return PdxlnIndexScanWithInlinedCondition(pexprChild, pexprScalarCond, dxl_properties, pdrgpcr, pdrgpdsBaseTables);
		default:
			GPOS_ASSERT(COperator::EopPhysicalDynamicBitmapTableScan == pexprChild->Pop()->Eopid());
			return PdxlnDynamicBitmapTableScan(pexprChild, pdrgpcr, pdrgpdsBaseTables, pexprScalarCond, dxl_properties);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDML
//
//	@doc:
//		Translate a DML operator
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDML
	(
	CExpression *pexpr,
	DrgPcr *,// pdrgpcr
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	ULONG action_colid = 0;
	ULONG oid_colid = 0;
	ULONG ctid_colid = 0;
	ULONG segid_colid = 0;

	// extract components
	CPhysicalDML *popDML = CPhysicalDML::PopConvert(pexpr->Pop());
	*pfDML = false;
	if (IMDId::EmdidGPDBCtas == popDML->Ptabdesc()->MDId()->Emdidt())
	{
		return PdxlnCTAS(pexpr, pdrgpdsBaseTables, pulNonGatherMotions, pfDML);
	}

	EdxlDmlType dml_type_dxl = Edxldmloptype(popDML->Edmlop());

	CExpression *pexprChild = (*pexpr)[0];
	CTableDescriptor *ptabdesc = popDML->Ptabdesc();
	DrgPcr *pdrgpcrSource = popDML->PdrgpcrSource();

	CColRef *pcrAction = popDML->PcrAction();
	GPOS_ASSERT(NULL != pcrAction);
	action_colid = pcrAction->UlId();

	CColRef *pcrOid = popDML->PcrTableOid();
	if (pcrOid != NULL)
	{
		oid_colid = pcrOid->UlId();
	}

	CColRef *pcrCtid = popDML->PcrCtid();
	CColRef *pcrSegmentId = popDML->PcrSegmentId();
	if (NULL != pcrCtid)
	{
		GPOS_ASSERT(NULL != pcrSegmentId);
		ctid_colid = pcrCtid->UlId();
		segid_colid = pcrSegmentId->UlId();
	}

	CColRef *pcrTupleOid = popDML->PcrTupleOid();
	ULONG tuple_oid = 0;
	BOOL preserve_oids = false;
	if (NULL != pcrTupleOid)
	{
		preserve_oids = true;
		tuple_oid = pcrTupleOid->UlId();
	}

	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcrSource, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	CDXLTableDescr *table_descr = MakeDXLTableDescr(ptabdesc, NULL /*pdrgpcrOutput*/);
	ULongPtrArray *pdrgpul = CUtils::Pdrgpul(m_memory_pool, pdrgpcrSource);

	CDXLDirectDispatchInfo *dxl_direct_dispatch_info = GetDXLDirectDispatchInfo(pexpr);
	CDXLPhysicalDML *pdxlopDML = GPOS_NEW(m_memory_pool) CDXLPhysicalDML
									(
									m_memory_pool,
									dml_type_dxl,
									table_descr,
									pdrgpul,
									action_colid,
									oid_colid,
									ctid_colid,
									segid_colid,
									preserve_oids,
									tuple_oid,
									dxl_direct_dispatch_info,
									popDML->IsInputSortReq()
									);

	// project list
	CColRefSet *pcrsOutput = pexpr->Prpp()->PcrsRequired();
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrSource);

	CDXLNode *pdxlnDML = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopDML);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexpr);
	pdxlnDML->SetProperties(dxl_properties);

	pdxlnDML->AddChild(pdxlnPrL);
	pdxlnDML->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnDML->GetOperator()->AssertValid(pdxlnDML, false /* validate_children */);
#endif
	*pfDML = true;

	return pdxlnDML;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnCTAS
//
//	@doc:
//		Translate a CTAS expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnCTAS
	(
	CExpression *pexpr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	CPhysicalDML *popDML = CPhysicalDML::PopConvert(pexpr->Pop());
	GPOS_ASSERT(CLogicalDML::EdmlInsert == popDML->Edmlop());

	CExpression *pexprChild = (*pexpr)[0];
	CTableDescriptor *ptabdesc = popDML->Ptabdesc();
	DrgPcr *pdrgpcrSource = popDML->PdrgpcrSource();
	CMDRelationCtasGPDB *pmdrel = (CMDRelationCtasGPDB *) m_pmda->Pmdrel(ptabdesc->MDId());

	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcrSource, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, true /*fRoot*/);

	ULongPtrArray *pdrgpul = CUtils::Pdrgpul(m_memory_pool, pdrgpcrSource);

	pmdrel->Pdxlctasopt()->AddRef();

	const ULONG ulColumns = ptabdesc->UlColumns();

	IntPtrArray *pdrgpiVarTypeMod = pmdrel->PdrgpiVarTypeMod();
	GPOS_ASSERT(ulColumns == pdrgpiVarTypeMod->Size());

	// translate col descriptors
	ColumnDescrDXLArray *pdrgpdxlcd = GPOS_NEW(m_memory_pool) ColumnDescrDXLArray(m_memory_pool);
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const CColumnDescriptor *pcd = ptabdesc->Pcoldesc(ul);

		CMDName *pmdnameCol = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pcd->Name().Pstr());
		CColRef *pcr = m_pcf->PcrCreate(pcd->Pmdtype(), pcd->TypeModifier(), pcd->Name());

		// use the col ref id for the corresponding output output column as 
		// colid for the dxl column
		CMDIdGPDB *pmdidColType = CMDIdGPDB::PmdidConvert(pcr->Pmdtype()->MDId());
		pmdidColType->AddRef();

		CDXLColDescr *pdxlcd = GPOS_NEW(m_memory_pool) CDXLColDescr
											(
											m_memory_pool,
											pmdnameCol,
											pcr->UlId(),
											pcd->AttrNum(),
											pmdidColType,
											pcr->TypeModifier(),
											false /* fdropped */,
											pcd->Width()
											);

		pdrgpdxlcd->Append(pdxlcd);
	}

	ULongPtrArray *pdrgpulDistr = NULL;
	if (IMDRelation::EreldistrHash == pmdrel->Ereldistribution())
	{
		pdrgpulDistr = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
		const ULONG ulDistrCols = pmdrel->UlDistrColumns();
		for (ULONG ul = 0; ul < ulDistrCols; ul++)
		{
			const IMDColumn *pmdcol = pmdrel->PmdcolDistrColumn(ul);
			INT iAttno = pmdcol->AttrNum();
			GPOS_ASSERT(0 < iAttno);
			pdrgpulDistr->Append(GPOS_NEW(m_memory_pool) ULONG(iAttno - 1));
		}
	}

	CMDName *pmdnameSchema = NULL;
	if (NULL != pmdrel->PmdnameSchema())
	{
		pmdnameSchema = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pmdrel->PmdnameSchema()->GetMDName());
	}

	pdrgpiVarTypeMod->AddRef();
	CDXLPhysicalCTAS *pdxlopCTAS = GPOS_NEW(m_memory_pool) CDXLPhysicalCTAS
									(
									m_memory_pool,
									pmdnameSchema,
									GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pmdrel->Mdname().GetMDName()),
									pdrgpdxlcd,
									pmdrel->Pdxlctasopt(),
									pmdrel->Ereldistribution(),
									pdrgpulDistr,
									pmdrel->FTemporary(),
									pmdrel->FHasOids(),
									pmdrel->Erelstorage(),
									pdrgpul,
									pdrgpiVarTypeMod
									);

	CDXLNode *pdxlnCTAS = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopCTAS);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexpr);
	pdxlnCTAS->SetProperties(dxl_properties);

	CColRefSet *pcrsOutput = pexpr->Prpp()->PcrsRequired();
	CDXLNode *pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrSource);

	pdxlnCTAS->AddChild(pdxlnPrL);
	pdxlnCTAS->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnCTAS->GetOperator()->AssertValid(pdxlnCTAS, false /* validate_children */);
#endif
	return pdxlnCTAS;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetDXLDirectDispatchInfo
//
//	@doc:
//		Return the direct dispatch info spec for the possible values of the distribution
//		key in a DML insert statement. Returns NULL if values are not constant.
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo *
CTranslatorExprToDXL::GetDXLDirectDispatchInfo
	(
	CExpression *pexprDML
	)
{
	GPOS_ASSERT(NULL != pexprDML);

	CPhysicalDML *popDML = CPhysicalDML::PopConvert(pexprDML->Pop());
	CTableDescriptor *ptabdesc = popDML->Ptabdesc();
	const DrgPcoldesc *pdrgpcoldescDist = ptabdesc->PdrgpcoldescDist();

	if (CLogicalDML::EdmlInsert != popDML->Edmlop() ||
		IMDRelation::EreldistrHash != ptabdesc->Ereldistribution() ||
		1 < pdrgpcoldescDist->Size())
	{
		// directed dispatch only supported for insert statements on hash-distributed tables 
		// with a single distribution column
		return NULL;
	}


	GPOS_ASSERT(1 == pdrgpcoldescDist->Size());
	CColumnDescriptor *pcoldesc = (*pdrgpcoldescDist)[0];
	ULONG ulPos = ptabdesc->UlPos(pcoldesc, ptabdesc->Pdrgpcoldesc());
	GPOS_ASSERT(ulPos < ptabdesc->Pdrgpcoldesc()->Size() && "Column not found");

	CColRef *pcrDistrCol = (*popDML->PdrgpcrSource())[ulPos];
	CPropConstraint *ppc = CDrvdPropRelational::Pdprel((*pexprDML)[0]->Pdp(CDrvdProp::EptRelational))->Ppc();

	if (NULL == ppc->Pcnstr())
	{
		return NULL;
	}

	CConstraint *pcnstrDistrCol = ppc->Pcnstr()->Pcnstr(m_memory_pool, pcrDistrCol);
	if (!CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		CRefCount::SafeRelease(pcnstrDistrCol);
		return NULL;
	}

	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());

	CConstraintInterval *pci = dynamic_cast<CConstraintInterval *>(pcnstrDistrCol);
	GPOS_ASSERT(1 >= pci->Pdrgprng()->Size());

	DXLDatumArray *pdrgpdxldatum = GPOS_NEW(m_memory_pool) DXLDatumArray(m_memory_pool);
	CDXLDatum *datum_dxl = NULL;

	if (1 == pci->Pdrgprng()->Size())
	{
		const CRange *prng = (*pci->Pdrgprng())[0];
		datum_dxl = CTranslatorExprToDXLUtils::GetDatumVal(m_memory_pool, m_pmda, prng->PdatumLeft());
	}
	else
	{
		GPOS_ASSERT(pci->FIncludesNull());
		datum_dxl = pcrDistrCol->Pmdtype()->PdxldatumNull(m_memory_pool);
	}

	pdrgpdxldatum->Append(datum_dxl);

	pcnstrDistrCol->Release();

	DXLDatumArrays *pdrgpdrgpdxldatum = GPOS_NEW(m_memory_pool) DXLDatumArrays(m_memory_pool);
	pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
	return GPOS_NEW(m_memory_pool) CDXLDirectDispatchInfo(pdrgpdrgpdxldatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnSplit
//
//	@doc:
//		Translate a split operator
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnSplit
	(
	CExpression *pexpr,
	DrgPcr *, // pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(2 == pexpr->Arity());

	ULONG action_colid = 0;
	ULONG ctid_colid = 0;
	ULONG segid_colid = 0;
	ULONG tuple_oid = 0;

	// extract components
	CPhysicalSplit *popSplit = CPhysicalSplit::PopConvert(pexpr->Pop());

	CExpression *pexprChild = (*pexpr)[0];
	CExpression *pexprProjList = (*pexpr)[1];

	CColRef *pcrAction = popSplit->PcrAction();
	GPOS_ASSERT(NULL != pcrAction);
	action_colid = pcrAction->UlId();

	CColRef *pcrCtid = popSplit->PcrCtid();
	GPOS_ASSERT(NULL != pcrCtid);
	ctid_colid = pcrCtid->UlId();

	CColRef *pcrSegmentId = popSplit->PcrSegmentId();
	GPOS_ASSERT(NULL != pcrSegmentId);
	segid_colid = pcrSegmentId->UlId();

	CColRef *pcrTupleOid = popSplit->PcrTupleOid();
	BOOL preserve_oids = false;
	if (NULL != pcrTupleOid)
	{
		preserve_oids = true;
		tuple_oid = pcrTupleOid->UlId();
	}

	DrgPcr *pdrgpcrDelete = popSplit->PdrgpcrDelete();
	ULongPtrArray *delete_colid_array = CUtils::Pdrgpul(m_memory_pool, pdrgpcrDelete);

	DrgPcr *pdrgpcrInsert = popSplit->PdrgpcrInsert();
	ULongPtrArray *insert_colid_array = CUtils::Pdrgpul(m_memory_pool, pdrgpcrInsert);

	CColRefSet *pcrsRequired = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsRequired->Include(pdrgpcrInsert);
	pcrsRequired->Include(pdrgpcrDelete);
	DrgPcr *pdrgpcrRequired = pcrsRequired->Pdrgpcr(m_memory_pool);

	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcrRequired, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, true /*fRemap*/, false /*fRoot*/);
	pdrgpcrRequired->Release();
	pcrsRequired->Release();

	CDXLPhysicalSplit *pdxlopSplit = GPOS_NEW(m_memory_pool) CDXLPhysicalSplit
													(
													m_memory_pool,
													delete_colid_array,
													insert_colid_array,
													action_colid,
													ctid_colid,
													segid_colid,
													preserve_oids,
													tuple_oid
													);

	// project list
	CColRefSet *pcrsOutput = pexpr->Prpp()->PcrsRequired();
	CDXLNode *pdxlnPrL = PdxlnProjList(pexprProjList, pcrsOutput, pdrgpcrInsert);

	CDXLNode *pdxlnSplit = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopSplit);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexpr);
	pdxlnSplit->SetProperties(dxl_properties);

	pdxlnSplit->AddChild(pdxlnPrL);
	pdxlnSplit->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnSplit->GetOperator()->AssertValid(pdxlnSplit, false /* validate_children */);
#endif
	return pdxlnSplit;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnRowTrigger
//
//	@doc:
//		Translate a row trigger operator
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnRowTrigger
	(
	CExpression *pexpr,
	DrgPcr *, // pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(1 == pexpr->Arity());

	// extract components
	CPhysicalRowTrigger *popRowTrigger = CPhysicalRowTrigger::PopConvert(pexpr->Pop());

	CExpression *pexprChild = (*pexpr)[0];

	IMDId *pmdidRel = popRowTrigger->PmdidRel();
	pmdidRel->AddRef();

	INT iType = popRowTrigger->IType();

	CColRefSet *pcrsRequired = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	ULongPtrArray *pdrgpulOld = NULL;
	ULongPtrArray *pdrgpulNew = NULL;

	DrgPcr *pdrgpcrOld = popRowTrigger->PdrgpcrOld();
	if (NULL != pdrgpcrOld)
	{
		pdrgpulOld = CUtils::Pdrgpul(m_memory_pool, pdrgpcrOld);
		pcrsRequired->Include(pdrgpcrOld);
	}

	DrgPcr *pdrgpcrNew = popRowTrigger->PdrgpcrNew();
	if (NULL != pdrgpcrNew)
	{
		pdrgpulNew = CUtils::Pdrgpul(m_memory_pool, pdrgpcrNew);
		pcrsRequired->Include(pdrgpcrNew);
	}

	DrgPcr *pdrgpcrRequired = pcrsRequired->Pdrgpcr(m_memory_pool);
	CDXLNode *child_dxlnode = CreateDXLNode(pexprChild, pdrgpcrRequired, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);
	pdrgpcrRequired->Release();
	pcrsRequired->Release();

	CDXLPhysicalRowTrigger *pdxlopRowTrigger = GPOS_NEW(m_memory_pool) CDXLPhysicalRowTrigger
													(
													m_memory_pool,
													pmdidRel,
													iType,
													pdrgpulOld,
													pdrgpulNew
													);

	// project list
	CColRefSet *pcrsOutput = pexpr->Prpp()->PcrsRequired();
	CDXLNode *pdxlnPrL = NULL;
	if (NULL != pdrgpcrNew)
	{
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrNew);
	}
	else
	{
		pdxlnPrL = PdxlnProjList(pcrsOutput, pdrgpcrOld);
	}

	CDXLNode *pdxlnRowTrigger = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopRowTrigger);
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexpr);
	pdxlnRowTrigger->SetProperties(dxl_properties);

	pdxlnRowTrigger->AddChild(pdxlnPrL);
	pdxlnRowTrigger->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlnRowTrigger->GetOperator()->AssertValid(pdxlnRowTrigger, false /* validate_children */);
#endif
	return pdxlnRowTrigger;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Edxldmloptype
//
//	@doc:
//		Return the EdxlDmlType for a given DML op type
//
//---------------------------------------------------------------------------
EdxlDmlType
CTranslatorExprToDXL::Edxldmloptype
	(
	const CLogicalDML::EDMLOperator edmlop
	)
	const
{
	switch (edmlop)
	{
		case CLogicalDML::EdmlInsert:
			return Edxldmlinsert;

		case CLogicalDML::EdmlDelete:
			return Edxldmldelete;

		case CLogicalDML::EdmlUpdate:
			return Edxldmlupdate;

		default:
			GPOS_ASSERT(!"Unrecognized DML operation");
			return EdxldmlSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCmp
//
//	@doc:
//		Create a DXL scalar comparison node from an optimizer scalar comparison 
//		expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCmp
	(
	CExpression *pexprScCmp
	)
{
	GPOS_ASSERT(NULL != pexprScCmp);

	// extract components
	CExpression *pexprLeft = (*pexprScCmp)[0];
	CExpression *pexprRight = (*pexprScCmp)[1];

	// translate children expression
	CDXLNode *pdxlnLeft = PdxlnScalar(pexprLeft);
	CDXLNode *pdxlnRight = PdxlnScalar(pexprRight);

	CScalarCmp *popScCmp = CScalarCmp::PopConvert(pexprScCmp->Pop());

	GPOS_ASSERT(NULL != popScCmp);
	GPOS_ASSERT(NULL != popScCmp->Pstr());
	GPOS_ASSERT(NULL != popScCmp->Pstr()->GetBuffer());

	// construct a scalar comparison node
	IMDId *pmdid = popScCmp->PmdidOp();
	pmdid->AddRef();

	CWStringConst *pstrName = GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, popScCmp->Pstr()->GetBuffer());

	CDXLNode *pdxlnCmp = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarComp(m_memory_pool, pmdid, pstrName));

	// add children
	pdxlnCmp->AddChild(pdxlnLeft);
	pdxlnCmp->AddChild(pdxlnRight);

#ifdef GPOS_DEBUG
	pdxlnCmp->GetOperator()->AssertValid(pdxlnCmp, false /* validate_children */);
#endif

	return pdxlnCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScDistinctCmp
//
//	@doc:
//		Create a DXL scalar distinct comparison node from an optimizer scalar
//		is distinct from expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScDistinctCmp
	(
	CExpression *pexprScDist
	)
{
	GPOS_ASSERT(NULL != pexprScDist);

	// extract components
	CExpression *pexprLeft = (*pexprScDist)[0];
	CExpression *pexprRight = (*pexprScDist)[1];

	// translate children expression
	CDXLNode *pdxlnLeft = PdxlnScalar(pexprLeft);
	CDXLNode *pdxlnRight = PdxlnScalar(pexprRight);

	CScalarIsDistinctFrom *popScIDF = CScalarIsDistinctFrom::PopConvert(pexprScDist->Pop());

	// construct a scalar distinct comparison node
	IMDId *pmdid = popScIDF->PmdidOp();
	pmdid->AddRef();

	CDXLNode *pdxlnDistCmp = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarDistinctComp(m_memory_pool, pmdid));

	// add children
	pdxlnDistCmp->AddChild(pdxlnLeft);
	pdxlnDistCmp->AddChild(pdxlnRight);

#ifdef GPOS_DEBUG
	pdxlnDistCmp->GetOperator()->AssertValid(pdxlnDistCmp, false /* validate_children */);
#endif

	return pdxlnDistCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScOp
//
//	@doc:
//		Create a DXL scalar op expr node from an optimizer scalar op expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScOp
	(
	CExpression *pexprOp
	)
{
	GPOS_ASSERT(NULL != pexprOp && ((1 == pexprOp->Arity()) || (2 == pexprOp->Arity())));
	CScalarOp *pscop = CScalarOp::PopConvert(pexprOp->Pop());

	// construct a scalar opexpr node
	CWStringConst *pstrName = GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pscop->Pstr()->GetBuffer());

	IMDId *pmdidOp = pscop->PmdidOp();
	pmdidOp->AddRef();

	IMDId *pmdidReturnType = pscop->PmdidReturnType();
	if (NULL != pmdidReturnType)
	{
		pmdidReturnType->AddRef();
	}

	CDXLNode *pdxlnOpExpr = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOpExpr(m_memory_pool, pmdidOp, pmdidReturnType, pstrName));

	TranslateScalarChildren(pexprOp, pdxlnOpExpr);

#ifdef GPOS_DEBUG
	pdxlnOpExpr->GetOperator()->AssertValid(pdxlnOpExpr, false /* validate_children */);
#endif

	return pdxlnOpExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScBoolExpr
//
//	@doc:
//		Create a DXL scalar bool expression node from an optimizer scalar log op
//		expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScBoolExpr
	(
	CExpression *pexprScBoolOp
	)
{
	GPOS_ASSERT(NULL != pexprScBoolOp);
	CScalarBoolOp *popScBoolOp = CScalarBoolOp::PopConvert(pexprScBoolOp->Pop());
	EdxlBoolExprType edxlbooltype = Edxlbooltype(popScBoolOp->Eboolop());

#ifdef GPOS_DEBUG
	if(CScalarBoolOp::EboolopNot == popScBoolOp->Eboolop())
	{
		GPOS_ASSERT(1 == pexprScBoolOp->Arity());
	}
	else
	{
		GPOS_ASSERT(2 <= pexprScBoolOp->Arity());
	}
#endif // GPOS_DEBUG

	CDXLNode *pdxlnBoolExpr = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBoolExpr(m_memory_pool, edxlbooltype));

	TranslateScalarChildren(pexprScBoolOp, pdxlnBoolExpr);

#ifdef GPOS_DEBUG
	pdxlnBoolExpr->GetOperator()->AssertValid(pdxlnBoolExpr, false /* validate_children */);
#endif

	return pdxlnBoolExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Edxlbooltype
//
//	@doc:
//		Return the EdxlBoolExprType for a given scalar logical op type
//
//---------------------------------------------------------------------------
EdxlBoolExprType
CTranslatorExprToDXL::Edxlbooltype
	(
	const CScalarBoolOp::EBoolOperator eboolop
	)
	const
{
	switch (eboolop)
	{
		case CScalarBoolOp::EboolopNot:
			return Edxlnot;

		case CScalarBoolOp::EboolopAnd:
			return Edxland;

		case CScalarBoolOp::EboolopOr:
			return Edxlor;

		default:
			GPOS_ASSERT(!"Unrecognized boolean expression type");
			return EdxlBoolExprTypeSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScId
//
//	@doc:
//		Create a DXL scalar identifier node from an optimizer scalar id expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScId
	(
	CExpression *pexprIdent
	)
{
	GPOS_ASSERT(NULL != pexprIdent);

	CScalarIdent *popScId = CScalarIdent::PopConvert(pexprIdent->Pop());
	CColRef *pcr = const_cast<CColRef*>(popScId->Pcr());

	return CTranslatorExprToDXLUtils::PdxlnIdent(m_memory_pool, m_phmcrdxln, m_phmcrdxlnIndexLookup, pcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScFuncExpr
//
//	@doc:
//		Create a DXL scalar func expr node from an optimizer scalar func expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScFuncExpr
	(
	CExpression *pexprFunc
	)
{
	GPOS_ASSERT(NULL != pexprFunc);

	CScalarFunc *popScFunc = CScalarFunc::PopConvert(pexprFunc->Pop());

	IMDId *mdid_func = popScFunc->FuncMdId();
	mdid_func->AddRef();

	IMDId *mdid_return_type = popScFunc->MDIdType();
	mdid_return_type->AddRef();

	const IMDFunction *pmdfunc = m_pmda->Pmdfunc(mdid_func);

	CDXLNode *pdxlnFuncExpr = GPOS_NEW(m_memory_pool) CDXLNode
											(
											m_memory_pool,
											GPOS_NEW(m_memory_pool) CDXLScalarFuncExpr(m_memory_pool, mdid_func, mdid_return_type, popScFunc->TypeModifier(), pmdfunc->FReturnsSet())
											);

	// translate children
	TranslateScalarChildren(pexprFunc, pdxlnFuncExpr);

	return pdxlnFuncExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScWindowFuncExpr
//
//	@doc:
//		Create a DXL scalar window ref node from an optimizer scalar window
//		function expr
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScWindowFuncExpr
	(
	CExpression *pexprWindowFunc
	)
{
	GPOS_ASSERT(NULL != pexprWindowFunc);

	CScalarWindowFunc *popScWindowFunc = CScalarWindowFunc::PopConvert(pexprWindowFunc->Pop());

	IMDId *mdid_func = popScWindowFunc->FuncMdId();
	mdid_func->AddRef();

	IMDId *mdid_return_type = popScWindowFunc->MDIdType();
	mdid_return_type->AddRef();

	EdxlWinStage edxlwinstage = Ews(popScWindowFunc->Ews());
	CDXLScalarWindowRef *pdxlopWindowref = GPOS_NEW(m_memory_pool) CDXLScalarWindowRef
															(
															m_memory_pool,
															mdid_func,
															mdid_return_type,
															popScWindowFunc->FDistinct(),
															popScWindowFunc->FStarArg(),
															popScWindowFunc->FSimpleAgg(),
															edxlwinstage,
															0 /* ulWinspecPosition */
															);

	CDXLNode *pdxlnWindowRef = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopWindowref);

	// translate children
	TranslateScalarChildren(pexprWindowFunc, pdxlnWindowRef);

	return pdxlnWindowRef;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Ews
//
//	@doc:
//		Get the DXL representation of the window stage
//
//---------------------------------------------------------------------------
EdxlWinStage
CTranslatorExprToDXL::Ews
	(
	CScalarWindowFunc::EWinStage ews
	)
	const
{
	ULONG rgrgulMapping[][2] =
	{
		{EdxlwinstageImmediate, CScalarWindowFunc::EwsImmediate},
		{EdxlwinstagePreliminary, CScalarWindowFunc::EwsPreliminary},
		{EdxlwinstageRowKey, CScalarWindowFunc::EwsRowKey}
	};
#ifdef GPOS_DEBUG
	const ULONG arity = GPOS_ARRAY_SIZE(rgrgulMapping);
	GPOS_ASSERT(arity > (ULONG) ews);
#endif
	ULONG *pulElem = rgrgulMapping[(ULONG) ews];
	EdxlWinStage edxlws = (EdxlWinStage) pulElem[0];

	return edxlws;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScAggref
//
//	@doc:
//		Create a DXL scalar aggref node from an optimizer scalar agg func expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScAggref
	(
	CExpression *pexprAggFunc
	)
{
	GPOS_ASSERT(NULL != pexprAggFunc);

	CScalarAggFunc *popScAggFunc = CScalarAggFunc::PopConvert(pexprAggFunc->Pop());
	IMDId *pmdidAggFunc = popScAggFunc->MDId();
	pmdidAggFunc->AddRef();

	IMDId *pmdidResolvedRetType = NULL;
	if (popScAggFunc->FHasAmbiguousReturnType())
	{
		// Agg has an ambiguous return type, use the resolved type instead
		pmdidResolvedRetType = popScAggFunc->MDIdType();
		pmdidResolvedRetType->AddRef();
	}

	EdxlAggrefStage edxlaggrefstage = EdxlaggstageNormal;

	if (popScAggFunc->FGlobal() && popScAggFunc->FSplit())
	{
		edxlaggrefstage = EdxlaggstageFinal;
	}
	else if (EaggfuncstageIntermediate == popScAggFunc->Eaggfuncstage())
	{
		edxlaggrefstage = EdxlaggstageIntermediate;
	}
	else if (!popScAggFunc->FGlobal())
	{
		edxlaggrefstage = EdxlaggstagePartial;
	}

	CDXLScalarAggref *pdxlopAggRef = GPOS_NEW(m_memory_pool) CDXLScalarAggref
												(
												m_memory_pool,
												pmdidAggFunc,
												pmdidResolvedRetType,
												popScAggFunc->FDistinct(),
												edxlaggrefstage
												);

	CDXLNode *pdxlnAggref = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopAggRef);
	TranslateScalarChildren(pexprAggFunc, pdxlnAggref);

	return pdxlnAggref;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScIfStmt
//
//	@doc:
//		Create a DXL scalar if node from an optimizer scalar if expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScIfStmt
	(
	CExpression *pexprIfStmt
	)
{
	GPOS_ASSERT(NULL != pexprIfStmt);

	GPOS_ASSERT(3 == pexprIfStmt->Arity());

	CScalarIf *popScIf = CScalarIf::PopConvert(pexprIfStmt->Pop());

	IMDId *mdid_type = popScIf->MDIdType();
	mdid_type->AddRef();

	CDXLNode *pdxlnIfStmt = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarIfStmt(m_memory_pool, mdid_type));
	TranslateScalarChildren(pexprIfStmt, pdxlnIfStmt);

	return pdxlnIfStmt;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScSwitch
//
//	@doc:
//		Create a DXL scalar switch node from an optimizer scalar switch expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScSwitch
	(
	CExpression *pexprSwitch
	)
{
	GPOS_ASSERT(NULL != pexprSwitch);
	GPOS_ASSERT(1 < pexprSwitch->Arity());
	CScalarSwitch *pop = CScalarSwitch::PopConvert(pexprSwitch->Pop());

	IMDId *mdid_type = pop->MDIdType();
	mdid_type->AddRef();

	CDXLNode *pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSwitch(m_memory_pool, mdid_type));
	TranslateScalarChildren(pexprSwitch, pdxln);

	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScSwitchCase
//
//	@doc:
//		Create a DXL scalar switch case node from an optimizer scalar switch
//		case expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScSwitchCase
	(
	CExpression *pexprSwitchCase
	)
{
	GPOS_ASSERT(NULL != pexprSwitchCase);
	GPOS_ASSERT(2 == pexprSwitchCase->Arity());

	CDXLNode *pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSwitchCase(m_memory_pool));
	TranslateScalarChildren(pexprSwitchCase, pdxln);

	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScNullIf
//
//	@doc:
//		Create a DXL scalar nullif node from an optimizer scalar
//		nullif expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScNullIf
	(
	CExpression *pexprScNullIf
	)
{
	GPOS_ASSERT(NULL != pexprScNullIf);

	CScalarNullIf *pop = CScalarNullIf::PopConvert(pexprScNullIf->Pop());

	IMDId *pmdid = pop->PmdidOp();
	pmdid->AddRef();

	IMDId *mdid_type = pop->MDIdType();
	mdid_type->AddRef();

	CDXLScalarNullIf *dxl_op = GPOS_NEW(m_memory_pool) CDXLScalarNullIf(m_memory_pool, pmdid, mdid_type);
	CDXLNode *pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	TranslateScalarChildren(pexprScNullIf, pdxln);

	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCaseTest
//
//	@doc:
//		Create a DXL scalar case test node from an optimizer scalar case test
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCaseTest
	(
	CExpression *pexprScCaseTest
	)
{
	GPOS_ASSERT(NULL != pexprScCaseTest);
	CScalarCaseTest *pop = CScalarCaseTest::PopConvert(pexprScCaseTest->Pop());

	IMDId *mdid_type = pop->MDIdType();
	mdid_type->AddRef();

	CDXLScalarCaseTest *dxl_op = GPOS_NEW(m_memory_pool) CDXLScalarCaseTest(m_memory_pool, mdid_type);

	return GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScNullTest
//
//	@doc:
//		Create a DXL scalar null test node from an optimizer scalar null test expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScNullTest
	(
	CExpression *pexprNullTest
	)
{
	GPOS_ASSERT(NULL != pexprNullTest);

	CDXLNode *pdxlnNullTest = GPOS_NEW(m_memory_pool) CDXLNode
											(
											m_memory_pool,
											GPOS_NEW(m_memory_pool) CDXLScalarNullTest(m_memory_pool, true /* fIsNull */));

	// translate child
	GPOS_ASSERT(1 == pexprNullTest->Arity());

	CExpression *pexprChild = (*pexprNullTest)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnNullTest->AddChild(child_dxlnode);

	return pdxlnNullTest;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScBooleanTest
//
//	@doc:
//		Create a DXL scalar null test node from an optimizer scalar null test expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScBooleanTest
	(
	CExpression *pexprScBooleanTest
	)
{
	GPOS_ASSERT(NULL != pexprScBooleanTest);
	GPOS_ASSERT(1 == pexprScBooleanTest->Arity());

	const ULONG rgulBoolTestMapping[][2] =
	{
		{CScalarBooleanTest::EbtIsTrue, EdxlbooleantestIsTrue},
		{CScalarBooleanTest::EbtIsNotTrue, EdxlbooleantestIsNotTrue},
		{CScalarBooleanTest::EbtIsFalse, EdxlbooleantestIsFalse},
		{CScalarBooleanTest::EbtIsNotFalse, EdxlbooleantestIsNotFalse},
		{CScalarBooleanTest::EbtIsUnknown, EdxlbooleantestIsUnknown},
		{CScalarBooleanTest::EbtIsNotUnknown, EdxlbooleantestIsNotUnknown},
	};

	CScalarBooleanTest *popBoolTest = CScalarBooleanTest::PopConvert(pexprScBooleanTest->Pop());
	EdxlBooleanTestType edxlbooltest = (EdxlBooleanTestType) (rgulBoolTestMapping[popBoolTest->Ebt()][1]);
	CDXLNode *pdxlnScBooleanTest = GPOS_NEW(m_memory_pool) CDXLNode
											(
											m_memory_pool,
											GPOS_NEW(m_memory_pool) CDXLScalarBooleanTest(m_memory_pool, edxlbooltest)
											);

	// translate child
	CExpression *pexprChild = (*pexprScBooleanTest)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnScBooleanTest->AddChild(child_dxlnode);

	return pdxlnScBooleanTest;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCoalesce
//
//	@doc:
//		Create a DXL scalar coalesce node from an optimizer scalar coalesce expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCoalesce
	(
	CExpression *pexprCoalesce
	)
{
	GPOS_ASSERT(NULL != pexprCoalesce);
	GPOS_ASSERT(0 < pexprCoalesce->Arity());
	CScalarCoalesce *popScCoalesce = CScalarCoalesce::PopConvert(pexprCoalesce->Pop());

	IMDId *mdid_type = popScCoalesce->MDIdType();
	mdid_type->AddRef();

	CDXLNode *pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarCoalesce(m_memory_pool, mdid_type));
	TranslateScalarChildren(pexprCoalesce, pdxln);

	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScMinMax
//
//	@doc:
//		Create a DXL scalar MinMax node from an optimizer scalar MinMax expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScMinMax
	(
	CExpression *pexprMinMax
	)
{
	GPOS_ASSERT(NULL != pexprMinMax);
	GPOS_ASSERT(0 < pexprMinMax->Arity());
	CScalarMinMax *popScMinMax = CScalarMinMax::PopConvert(pexprMinMax->Pop());

	CScalarMinMax::EScalarMinMaxType esmmt = popScMinMax->Esmmt();
	GPOS_ASSERT(CScalarMinMax::EsmmtMin == esmmt || CScalarMinMax::EsmmtMax == esmmt);

	CDXLScalarMinMax::EdxlMinMaxType emmt = CDXLScalarMinMax::EmmtMin;
	if (CScalarMinMax::EsmmtMax == esmmt)
	{
		emmt = CDXLScalarMinMax::EmmtMax;
	}

	IMDId *mdid_type = popScMinMax->MDIdType();
	mdid_type->AddRef();

	CDXLNode *pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarMinMax(m_memory_pool, mdid_type, emmt));
	TranslateScalarChildren(pexprMinMax, pdxln);

	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::TranslateScalarChildren
//
//	@doc:
//		Translate expression children and add them as children of the DXL node
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXL::TranslateScalarChildren
	(
	CExpression *pexpr,
	CDXLNode *pdxln
	)
{
	const ULONG arity = pexpr->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprChild = (*pexpr)[ul];
		CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
		pdxln->AddChild(child_dxlnode);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCast
//
//	@doc:
//		Create a DXL scalar relabel type node from an
//		optimizer scalar relabel type expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCast
	(
	CExpression *pexprCast
	)
{
	GPOS_ASSERT(NULL != pexprCast);
	CScalarCast *popScCast = CScalarCast::PopConvert(pexprCast->Pop());

	IMDId *pmdid = popScCast->MDIdType();
	pmdid->AddRef();

	IMDId *mdid_func = popScCast->FuncMdId();
	mdid_func->AddRef();

	CDXLNode *pdxlnCast = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarCast(m_memory_pool, pmdid, mdid_func));

	// translate child
	GPOS_ASSERT(1 == pexprCast->Arity());
	CExpression *pexprChild = (*pexprCast)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCast->AddChild(child_dxlnode);

	return pdxlnCast;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCoerceToDomain
//
//	@doc:
//		Create a DXL scalar coerce node from an optimizer scalar coerce expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCoerceToDomain
	(
	CExpression *pexprCoerce
	)
{
	GPOS_ASSERT(NULL != pexprCoerce);
	CScalarCoerceToDomain *popScCoerce = CScalarCoerceToDomain::PopConvert(pexprCoerce->Pop());

	IMDId *pmdid = popScCoerce->MDIdType();
	pmdid->AddRef();


	CDXLNode *pdxlnCoerce =
		GPOS_NEW(m_memory_pool) CDXLNode
			(
			m_memory_pool,
			GPOS_NEW(m_memory_pool) CDXLScalarCoerceToDomain
					(
					m_memory_pool,
					pmdid,
					popScCoerce->TypeModifier(),
					(EdxlCoercionForm) popScCoerce->Ecf(), // map Coercion Form directly based on position in enum
					popScCoerce->ILoc()
					)
			);

	// translate child
	GPOS_ASSERT(1 == pexprCoerce->Arity());
	CExpression *pexprChild = (*pexprCoerce)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCoerce->AddChild(child_dxlnode);

	return pdxlnCoerce;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScCoerceViaIO
//
//	@doc:
//		Create a DXL scalar coerce node from an optimizer scalar coerce expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScCoerceViaIO
	(
	CExpression *pexprCoerce
	)
{
	GPOS_ASSERT(NULL != pexprCoerce);
	CScalarCoerceViaIO *popScCerce = CScalarCoerceViaIO::PopConvert(pexprCoerce->Pop());

	IMDId *pmdid = popScCerce->MDIdType();
	pmdid->AddRef();


	CDXLNode *pdxlnCoerce =
		GPOS_NEW(m_memory_pool) CDXLNode
			(
			m_memory_pool,
			GPOS_NEW(m_memory_pool) CDXLScalarCoerceViaIO
					(
					m_memory_pool,
					pmdid,
					popScCerce->TypeModifier(),
					(EdxlCoercionForm) popScCerce->Ecf(), // map Coercion Form directly based on position in enum
					popScCerce->ILoc()
					)
			);

	// translate child
	GPOS_ASSERT(1 == pexprCoerce->Arity());
	CExpression *pexprChild = (*pexprCoerce)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnCoerce->AddChild(child_dxlnode);

	return pdxlnCoerce;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScArrayCoerceExpr
//
//	@doc:
//		Create a DXL node from an optimizer scalar array coerce expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScArrayCoerceExpr
	(
	CExpression *pexprArrayCoerceExpr
	)
{
	GPOS_ASSERT(NULL != pexprArrayCoerceExpr);
	CScalarArrayCoerceExpr *popScArrayCoerceExpr = CScalarArrayCoerceExpr::PopConvert(pexprArrayCoerceExpr->Pop());

	IMDId *pmdidElemFunc = popScArrayCoerceExpr->PmdidElementFunc();
	pmdidElemFunc->AddRef();
	IMDId *pmdid = popScArrayCoerceExpr->MDIdType();
	pmdid->AddRef();

	CDXLNode *pdxlnArrayCoerceExpr =
		GPOS_NEW(m_memory_pool) CDXLNode
			(
			m_memory_pool,
			GPOS_NEW(m_memory_pool) CDXLScalarArrayCoerceExpr
					(
					m_memory_pool,
					pmdidElemFunc,
					pmdid,
					popScArrayCoerceExpr->TypeModifier(),
					popScArrayCoerceExpr->FIsExplicit(),
					(EdxlCoercionForm) popScArrayCoerceExpr->Ecf(), // map Coercion Form directly based on position in enum
					popScArrayCoerceExpr->ILoc()
					)
			);

	// translate child
	GPOS_ASSERT(1 == pexprArrayCoerceExpr->Arity());
	CExpression *pexprChild = (*pexprArrayCoerceExpr)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	pdxlnArrayCoerceExpr->AddChild(child_dxlnode);

	return pdxlnArrayCoerceExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetWindowFrame
//
//	@doc:
//		Translate a window frame
//
//---------------------------------------------------------------------------
CDXLWindowFrame *
CTranslatorExprToDXL::GetWindowFrame
	(
	CWindowFrame *pwf
	)
{
	GPOS_ASSERT(NULL != pwf);

	if (CWindowFrame::IsEmpty(pwf))
	{
		// an empty frame is translated as 'no frame'
		return NULL;
	}

	// mappings for frame info in expression and dxl worlds
	const ULONG rgulSpecMapping[][2] =
	{
		{CWindowFrame::EfsRows, EdxlfsRow},
		{CWindowFrame::EfsRange, EdxlfsRange}
	};

	const ULONG rgulBoundaryMapping[][2] =
	{
		{CWindowFrame::EfbUnboundedPreceding, EdxlfbUnboundedPreceding},
		{CWindowFrame::EfbBoundedPreceding, EdxlfbBoundedPreceding},
		{CWindowFrame::EfbCurrentRow, EdxlfbCurrentRow},
		{CWindowFrame::EfbUnboundedFollowing, EdxlfbUnboundedFollowing},
		{CWindowFrame::EfbBoundedFollowing, EdxlfbBoundedFollowing},
		{CWindowFrame::EfbDelayedBoundedPreceding, EdxlfbDelayedBoundedPreceding},
		{CWindowFrame::EfbDelayedBoundedFollowing, EdxlfbDelayedBoundedFollowing}
	};

	const ULONG rgulExclusionStrategyMapping[][2] =
	{
		{CWindowFrame::EfesNone, EdxlfesNone},
		{CWindowFrame::EfesNulls, EdxlfesNulls},
		{CWindowFrame::EfesCurrentRow, EdxlfesCurrentRow},
		{CWindowFrame::EfseMatchingOthers, EdxlfesGroup},
		{CWindowFrame::EfesTies, EdxlfesTies}
	};

	EdxlFrameSpec edxlfs = (EdxlFrameSpec) (rgulSpecMapping[pwf->Efs()][1]);
	EdxlFrameBoundary edxlfbLeading = (EdxlFrameBoundary) (rgulBoundaryMapping[pwf->EfbLeading()][1]);
	EdxlFrameBoundary edxlfbTrailing = (EdxlFrameBoundary) (rgulBoundaryMapping[pwf->EfbTrailing()][1]);
	EdxlFrameExclusionStrategy edxlfes = (EdxlFrameExclusionStrategy) (rgulExclusionStrategyMapping[pwf->Efes()][1]);

	// translate scalar expressions representing leading and trailing frame edges
	CDXLNode *pdxlnLeading = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarWindowFrameEdge(m_memory_pool, true /* fLeading */, edxlfbLeading));
	if (NULL != pwf->PexprLeading())
	{
		pdxlnLeading->AddChild(PdxlnScalar(pwf->PexprLeading()));
	}

	CDXLNode *pdxlnTrailing = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarWindowFrameEdge(m_memory_pool, false /* fLeading */, edxlfbTrailing));
	if (NULL != pwf->PexprTrailing())
	{
		pdxlnTrailing->AddChild(PdxlnScalar(pwf->PexprTrailing()));
	}

	return GPOS_NEW(m_memory_pool) CDXLWindowFrame(m_memory_pool, edxlfs, edxlfes, pdxlnLeading, pdxlnTrailing);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnWindow
//
//	@doc:
//		Create a DXL window node from physical sequence project expression.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnWindow
	(
	CExpression *pexprSeqPrj,
	DrgPcr *pdrgpcr,
	DrgPds *pdrgpdsBaseTables,
	ULONG *pulNonGatherMotions,
	BOOL *pfDML
	)
{
	GPOS_ASSERT(NULL != pexprSeqPrj);

	CPhysicalSequenceProject *popSeqPrj = CPhysicalSequenceProject::PopConvert(pexprSeqPrj->Pop());
	CDistributionSpec *pds = popSeqPrj->Pds();
	ULongPtrArray *pdrgpulColIds = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	DrgPexpr *pdrgpexprPartCol = NULL;
	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		CDistributionSpecHashed *pdshashed = CDistributionSpecHashed::PdsConvert(pds);
		pdrgpexprPartCol = const_cast<DrgPexpr *>(pdshashed->Pdrgpexpr());
		const ULONG ulSize = pdrgpexprPartCol->Size();
		for (ULONG ul = 0; ul < ulSize; ul++)
		{
			CExpression *pexpr = (*pdrgpexprPartCol)[ul];
			CScalarIdent *popScId = CScalarIdent::PopConvert(pexpr->Pop());
			pdrgpulColIds->Append(GPOS_NEW(m_memory_pool) ULONG(popScId->Pcr()->UlId()));
		}
	}

	// translate order specification and window frames into window keys
	CDXLWindowKeyArray *pdrgpdxlwk = GPOS_NEW(m_memory_pool) CDXLWindowKeyArray(m_memory_pool);
	DrgPos *pdrgpos = popSeqPrj->Pdrgpos();
	GPOS_ASSERT(NULL != pdrgpos);
	const ULONG ulOsSize = pdrgpos->Size();
	for (ULONG ul = 0; ul < ulOsSize; ul++)
	{
		CDXLWindowKey *pdxlwk = GPOS_NEW(m_memory_pool) CDXLWindowKey(m_memory_pool);
		CDXLNode *sort_col_list_dxl = GetSortColListDXL((*popSeqPrj->Pdrgpos())[ul]);
		pdxlwk->SetSortColList(sort_col_list_dxl);
		pdrgpdxlwk->Append(pdxlwk);
	}

	const ULONG ulFrames = popSeqPrj->Pdrgpwf()->Size();
	for (ULONG ul = 0; ul < ulFrames; ul++)
	{
		CDXLWindowFrame *window_frame = GetWindowFrame((*popSeqPrj->Pdrgpwf())[ul]);
		if (NULL != window_frame)
		{
			GPOS_ASSERT(ul <= ulOsSize);
			CDXLWindowKey *pdxlwk = (*pdrgpdxlwk)[ul];
			pdxlwk->SetWindowFrame(window_frame);
		}
	}

	// extract physical properties
	CDXLPhysicalProperties *dxl_properties = GetProperties(pexprSeqPrj);

	// translate relational child
	CDXLNode *child_dxlnode = CreateDXLNode((*pexprSeqPrj)[0], NULL /* pdrgpcr */, pdrgpdsBaseTables, pulNonGatherMotions, pfDML, false /*fRemap*/, false /*fRoot*/);

	GPOS_ASSERT(NULL != pexprSeqPrj->Prpp());
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pexprSeqPrj->Prpp()->PcrsRequired());
	if (NULL != pdrgpexprPartCol)
	{
		CColRefSet *pcrs = CUtils::PcrsExtractColumns(m_memory_pool, pdrgpexprPartCol);
		pcrsOutput->Include(pcrs);
		pcrs->Release();
	}
	for (ULONG ul = 0; ul < ulOsSize; ul++)
	{
		COrderSpec *pos = (*popSeqPrj->Pdrgpos())[ul];
		if (!pos->IsEmpty())
		{
			const CColRef *pcr = pos->Pcr(ul);
			pcrsOutput->Include(pcr);
		}
	}

	// translate project list expression
	CDXLNode *pdxlnPrL = PdxlnProjList((*pexprSeqPrj)[1], pcrsOutput, pdrgpcr);

	// create an empty one-time filter
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL /* pdxlnCond */);

	// construct a Window node	
	CDXLPhysicalWindow *pdxlopWindow = GPOS_NEW(m_memory_pool) CDXLPhysicalWindow(m_memory_pool, pdrgpulColIds, pdrgpdxlwk);
	CDXLNode *pdxlnWindow = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopWindow);
	pdxlnWindow->SetProperties(dxl_properties);

	// add children
	pdxlnWindow->AddChild(pdxlnPrL);
	pdxlnWindow->AddChild(filter_dxlnode);
	pdxlnWindow->AddChild(child_dxlnode);

#ifdef GPOS_DEBUG
	pdxlopWindow->AssertValid(pdxlnWindow, false /* validate_children */);
#endif

	pcrsOutput->Release();

	return pdxlnWindow;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArray
//
//	@doc:
//		Create a DXL array node from an optimizer array expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnArray
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CScalarArray *pop = CScalarArray::PopConvert(pexpr->Pop());

	IMDId *pmdidElem = pop->PmdidElem();
	pmdidElem->AddRef();

	IMDId *pmdidArray = pop->PmdidArray();
	pmdidArray->AddRef();

	CDXLNode *pdxlnArray =
			GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarArray
									(
									m_memory_pool,
									pmdidElem,
									pmdidArray,
									pop->FMultiDimensional()
									)
						);

	const ULONG arity = CUtils::UlScalarArrayArity(pexpr);

	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprChild = CUtils::PScalarArrayExprChildAt(m_memory_pool, pexpr, ul);
		CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
		pdxlnArray->AddChild(child_dxlnode);
		pexprChild->Release();
	}

	return pdxlnArray;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArrayRef
//
//	@doc:
//		Create a DXL arrayref node from an optimizer arrayref expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnArrayRef
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CScalarArrayRef *pop = CScalarArrayRef::PopConvert(pexpr->Pop());

	IMDId *pmdidElem = pop->PmdidElem();
	pmdidElem->AddRef();

	IMDId *pmdidArray = pop->PmdidArray();
	pmdidArray->AddRef();

	IMDId *pmdidReturn = pop->MDIdType();
	pmdidReturn->AddRef();

	CDXLNode *pdxlnArrayref =
			GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarArrayRef
									(
									m_memory_pool,
									pmdidElem,
									pop->TypeModifier(),
									pmdidArray,
									pmdidReturn
									)
						);

	TranslateScalarChildren(pexpr, pdxlnArrayref);

	return pdxlnArrayref;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArrayRefIndexList
//
//	@doc:
//		Create a DXL arrayref index list from an optimizer arrayref index list
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnArrayRefIndexList
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CScalarArrayRefIndexList *pop = CScalarArrayRefIndexList::PopConvert(pexpr->Pop());

	CDXLNode *pdxlnIndexlist =
			GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarArrayRefIndexList
									(
									m_memory_pool,
									Eilb(pop->Eilt())
									)
						);

	TranslateScalarChildren(pexpr, pdxlnIndexlist);

	return pdxlnIndexlist;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssertPredicate
//
//	@doc:
//		Create a DXL assert predicate from an optimizer assert predicate expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAssertPredicate
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);

	CDXLNode *pdxlnAssertConstraintList = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarAssertConstraintList(m_memory_pool));
	TranslateScalarChildren(pexpr, pdxlnAssertConstraintList);
	return pdxlnAssertConstraintList;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnAssertConstraint
//
//	@doc:
//		Create a DXL assert constraint from an optimizer assert constraint expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnAssertConstraint
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CScalarAssertConstraint *popAssertConstraint = CScalarAssertConstraint::PopConvert(pexpr->Pop());
	CWStringDynamic *pstrErrorMsg = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool, popAssertConstraint->PstrErrorMsg()->GetBuffer());

	CDXLNode *pdxlnAssertConstraint  = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarAssertConstraint(m_memory_pool, pstrErrorMsg));
	TranslateScalarChildren(pexpr, pdxlnAssertConstraint);
	return pdxlnAssertConstraint;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::Eilb
//
//	@doc:
// 		Translate the arrayref index list bound
//
//---------------------------------------------------------------------------
CDXLScalarArrayRefIndexList::EIndexListBound
CTranslatorExprToDXL::Eilb
	(
	const CScalarArrayRefIndexList::EIndexListType eilt
	)
{
	switch (eilt)
	{
		case CScalarArrayRefIndexList::EiltLower:
			return CDXLScalarArrayRefIndexList::EilbLower;

		case CScalarArrayRefIndexList::EiltUpper:
			return CDXLScalarArrayRefIndexList::EilbUpper;

		default:
			GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp, GPOS_WSZ_LIT("Invalid arrayref index bound"));
			return CDXLScalarArrayRefIndexList::EilbSentinel;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnArrayCmp
//
//	@doc:
//		Create a DXL array compare node from an optimizer array expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnArrayCmp
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != pexpr);
	CScalarArrayCmp *pop = CScalarArrayCmp::PopConvert(pexpr->Pop());

	IMDId *pmdidOp = pop->PmdidOp();
	pmdidOp->AddRef();

	const CWStringConst *pstrOpName = pop->Pstr();

	CScalarArrayCmp::EArrCmpType earrcmpt = pop->Earrcmpt();
	GPOS_ASSERT(CScalarArrayCmp::EarrcmpSentinel > earrcmpt);
	EdxlArrayCompType edxlarrcmpt = Edxlarraycomptypeall;
	if (CScalarArrayCmp::EarrcmpAny == earrcmpt)
	{
		edxlarrcmpt = Edxlarraycomptypeany;
	}

	CDXLNode *pdxlnArrayCmp =
			GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarArrayComp
									(
									m_memory_pool,
									pmdidOp,
									GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pstrOpName->GetBuffer()),
									edxlarrcmpt
									)
						);

	TranslateScalarChildren(pexpr, pdxlnArrayCmp);

	return pdxlnArrayCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnDMLAction
//
//	@doc:
//		Create a DXL DML action node from an optimizer action expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnDMLAction
	(
	CExpression *
#ifdef GPOS_DEBUG
	pexpr
#endif // GPOS_DEBUG
	)
{
	GPOS_ASSERT(NULL != pexpr);
	GPOS_ASSERT(COperator::EopScalarDMLAction == pexpr->Pop()->Eopid());

	return GPOS_NEW(m_memory_pool) CDXLNode
						(
						m_memory_pool,
						GPOS_NEW(m_memory_pool) CDXLScalarDMLAction(m_memory_pool)
						);

}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnScConst
//
//	@doc:
//		Create a DXL scalar constant node from an optimizer scalar const expr.
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnScConst
	(
	CExpression *pexprScConst
	)
{
	GPOS_ASSERT(NULL != pexprScConst);

	CScalarConst *popScConst = CScalarConst::PopConvert(pexprScConst->Pop());

	IDatum *pdatum = popScConst->Pdatum();
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDType *pmdtype = md_accessor->Pmdtype(pdatum->MDId());

	CDXLNode *pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pmdtype->PdxlopScConst(m_memory_pool, pdatum));

	return pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnFilter
//
//	@doc:
//		Create a DXL filter node containing the given scalar node as a child.
//		If the scalar node is NULL, a filter node with no children is returned
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnFilter
	(
	CDXLNode *pdxlnCond
	)
{
	CDXLNode *filter_dxlnode = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarFilter(m_memory_pool));
	if (NULL != pdxlnCond)
	{
		filter_dxlnode->AddChild(pdxlnCond);
	}

	return filter_dxlnode;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::MakeDXLTableDescr
//
//	@doc:
//		Create a DXL table descriptor from the corresponding optimizer structure
//
//---------------------------------------------------------------------------
CDXLTableDescr *
CTranslatorExprToDXL::MakeDXLTableDescr
	(
	const CTableDescriptor *ptabdesc,
	const DrgPcr *pdrgpcrOutput
	)
{
	GPOS_ASSERT(NULL != ptabdesc);
	GPOS_ASSERT_IMP(NULL != pdrgpcrOutput, ptabdesc->UlColumns() == pdrgpcrOutput->Size());

	// get tbl name
	CMDName *pmdnameTbl = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, ptabdesc->Name().Pstr());

	CMDIdGPDB *pmdid = CMDIdGPDB::PmdidConvert(ptabdesc->MDId());
	pmdid->AddRef();

	CDXLTableDescr *table_descr = GPOS_NEW(m_memory_pool) CDXLTableDescr(m_memory_pool, pmdid, pmdnameTbl, ptabdesc->GetExecuteAsUserId());

	const ULONG ulColumns = ptabdesc->UlColumns();
	// translate col descriptors
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const CColumnDescriptor *pcd = ptabdesc->Pcoldesc(ul);

		GPOS_ASSERT(NULL != pcd);

		CMDName *pmdnameCol = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pcd->Name().Pstr());

		// output col ref for the current col descrs
		CColRef *pcr = NULL;
		if (NULL != pdrgpcrOutput)
		{
			pcr = (*pdrgpcrOutput)[ul];
		}
		else
		{
			pcr = m_pcf->PcrCreate(pcd->Pmdtype(), pcd->TypeModifier(), pcd->Name());
		}

		// use the col ref id for the corresponding output output column as 
		// colid for the dxl column
		CMDIdGPDB *pmdidColType = CMDIdGPDB::PmdidConvert(pcr->Pmdtype()->MDId());
		pmdidColType->AddRef();

		CDXLColDescr *pdxlcd = GPOS_NEW(m_memory_pool) CDXLColDescr
											(
											m_memory_pool,
											pmdnameCol,
											pcr->UlId(),
											pcd->AttrNum(),
											pmdidColType,
											pcr->TypeModifier(),
											false /* fdropped */,
											pcd->Width()
											);

		table_descr->AddColumnDescr(pdxlcd);
	}

	return table_descr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetProperties
//
//	@doc:
//		Construct a DXL physical properties container with operator costs for
//		the given expression
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties *
CTranslatorExprToDXL::GetProperties
	(
	const CExpression *pexpr
	)
{

	// extract out rows from statistics object
	CWStringDynamic *rows_out_str = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool);
	const IStatistics *pstats = pexpr->Pstats();
	CDouble dRows = CStatistics::DDefaultRelationRows;

	// stats may not be present in artificially generated physical expression trees.
	// fill in default statistics
	if (NULL != pstats)
	{
		dRows = pstats->DRows();
	}

	if (CDistributionSpec::EdtReplicated == CDrvdPropPlan::Pdpplan(pexpr->Pdp(CDrvdProp::EptPlan))->Pds()->Edt())
	{
		// if distribution is replicated, multiply number of rows by number of segments
		ULONG ulSegments = COptCtxt::PoctxtFromTLS()->GetCostModel()->UlHosts();
		dRows = dRows * ulSegments;
	}

	rows_out_str->AppendFormat(GPOS_WSZ_LIT("%f"), dRows.Get());

	// extract our width from statistics object
	CDouble dWidth = CStatistics::DDefaultColumnWidth;
	CReqdPropPlan *prpp = pexpr->Prpp();
	CColRefSet *pcrs = prpp->PcrsRequired();
	ULongPtrArray *pdrgpulColIds = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	pcrs->ExtractColIds(m_memory_pool, pdrgpulColIds);
	CWStringDynamic *width_str = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool);

	if (NULL != pstats)
	{
		dWidth = pstats->DWidth(pdrgpulColIds);
	}
	pdrgpulColIds->Release();
	width_str->AppendFormat(GPOS_WSZ_LIT("%lld"), (LINT) dWidth.Get());

	// get the cost from expression node
	CWStringDynamic str(m_memory_pool);
	COstreamString oss (&str);
	oss << pexpr->Cost();

	CWStringDynamic *pstrStartupcost = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool, GPOS_WSZ_LIT("0"));
	CWStringDynamic *pstrTotalcost = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool, str.GetBuffer());

	CDXLOperatorCost *pdxlopcost = GPOS_NEW(m_memory_pool) CDXLOperatorCost(pstrStartupcost, pstrTotalcost, rows_out_str, width_str);
	CDXLPhysicalProperties *dxl_properties = GPOS_NEW(m_memory_pool) CDXLPhysicalProperties(pdxlopcost);

	return dxl_properties;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjList
//
//	@doc:
//		Translate the set of output col refs into a dxl project list.
//		If the given array of columns is not NULL, it specifies the order of the 
//		columns in the project list, otherwise any order is good
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjList
	(
	const CColRefSet *pcrsOutput,
	DrgPcr *pdrgpcr
	)
{
	GPOS_ASSERT(NULL != pcrsOutput);

	CDXLScalarProjList *pdxlopPrL = GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool);
	CDXLNode *pdxlnPrL = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopPrL);

	if (NULL != pdrgpcr)
	{
		CColRefSet *pcrs= GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);

		for (ULONG ul = 0; ul < pdrgpcr->Size(); ul++)
		{
			CColRef *pcr = (*pdrgpcr)[ul];

			CDXLNode *pdxlnPrEl = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcr);
			pdxlnPrL->AddChild(pdxlnPrEl);
			pcrs->Include(pcr);
		}

		// add the remaining required columns
		CColRefSetIter crsi(*pcrsOutput);
		while(crsi.Advance())
		{
			CColRef *pcr = crsi.Pcr();

			if (!pcrs->FMember(pcr))
			{
				CDXLNode *pdxlnPrEl = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcr);
				pdxlnPrL->AddChild(pdxlnPrEl);
				pcrs->Include(pcr);
			}
		}
		pcrs->Release();
	}
	else
	{
		// no order specified
		CColRefSetIter crsi(*pcrsOutput);
		while(crsi.Advance())
		{
			CColRef *pcr = crsi.Pcr();
			CDXLNode *pdxlnPrEl = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcr);
			pdxlnPrL->AddChild(pdxlnPrEl);
		}
	}

	return pdxlnPrL;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjList
//
//	@doc:
//		 Translate a project list expression into DXL project list node
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjList
	(
	const CExpression *pexprProjList,
	const CColRefSet *pcrsRequired,
	DrgPcr *pdrgpcr
	)
{
	if (NULL == pdrgpcr)
	{
		// no order specified
		return PdxlnProjList(pexprProjList, pcrsRequired);
	}

	// translate computed column expressions into DXL and index them on their col ids
	CHashMap<ULONG, CDXLNode, gpos::HashValue<ULONG>, gpos::Equals<ULONG>, CleanupDelete<ULONG>, CleanupRelease<CDXLNode> >
		*phmComputedColumns = GPOS_NEW(m_memory_pool) CHashMap<ULONG, CDXLNode, gpos::HashValue<ULONG>, gpos::Equals<ULONG>, CleanupDelete<ULONG>, CleanupRelease<CDXLNode> >(m_memory_pool);

	for (ULONG ul = 0; NULL != pexprProjList && ul < pexprProjList->Arity(); ul++)
	{
		CExpression *pexprProjElem = (*pexprProjList)[ul];

		// translate proj elem
		CDXLNode *pdxlnProjElem = PdxlnProjElem(pexprProjElem);

		const CScalarProjectElement *popScPrEl =
				CScalarProjectElement::PopConvert(pexprProjElem->Pop());

		ULONG *pulKey = GPOS_NEW(m_memory_pool) ULONG(popScPrEl->Pcr()->UlId());
#ifdef GPOS_DEBUG
		BOOL fInserted =
#endif // GPOS_DEBUG
		phmComputedColumns->Insert(pulKey, pdxlnProjElem);

		GPOS_ASSERT(fInserted);
	}

	// add required columns to the project list
	DrgPcr *pdrgpcrCopy = GPOS_NEW(m_memory_pool) DrgPcr(m_memory_pool);
	pdrgpcrCopy->AppendArray(pdrgpcr);
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pdrgpcr);
	CColRefSetIter crsi(*pcrsRequired);
	while (crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		if (!pcrsOutput->FMember(pcr))
		{
			pdrgpcrCopy->Append(pcr);
		}
	}

	// translate project list according to the specified order
	CDXLScalarProjList *pdxlopPrL = GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool);
	CDXLNode *proj_list_dxlnode = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopPrL);

	const ULONG ulCols = pdrgpcrCopy->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CColRef *pcr = (*pdrgpcrCopy)[ul];
		ULONG ulKey = pcr->UlId();
		CDXLNode *pdxlnProjElem = phmComputedColumns->Find(&ulKey);

		if (NULL == pdxlnProjElem)
		{
			// not a computed column
			pdxlnProjElem = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcr);
		}
		else
		{
			pdxlnProjElem->AddRef();
		}

		proj_list_dxlnode->AddChild(pdxlnProjElem);
	}

	// cleanup
	pdrgpcrCopy->Release();
	pcrsOutput->Release();
	phmComputedColumns->Release();

	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjList
//
//	@doc:
//		 Translate a project list expression into DXL project list node
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjList
	(
	const CExpression *pexprProjList,
	const CColRefSet *pcrsRequired
	)
{
	CDXLScalarProjList *pdxlopPrL = GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool);
	CDXLNode *proj_list_dxlnode = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopPrL);

	// create a copy of the required output columns
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool, *pcrsRequired);

	if (NULL != pexprProjList)
	{
		// translate defined columns from project list
		for (ULONG ul = 0; ul < pexprProjList->Arity(); ul++)
		{
			CExpression *pexprProjElem = (*pexprProjList)[ul];
			
			// translate proj elem
			CDXLNode *pdxlnProjElem = PdxlnProjElem(pexprProjElem);
			proj_list_dxlnode->AddChild(pdxlnProjElem);

			// exclude proj elem col ref from the output column set as it has been
			// processed already
			const CScalarProjectElement *popScPrEl =
					CScalarProjectElement::PopConvert(pexprProjElem->Pop());
			pcrsOutput->Exclude(popScPrEl->Pcr());
		}
	}
	
	// translate columns which remained after processing the project list: those
	// are columns passed from the level below
	CColRefSetIter crsi(*pcrsOutput);
	while(crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		CDXLNode *pdxlnPrEl = CTranslatorExprToDXLUtils::PdxlnProjElem(m_memory_pool, m_phmcrdxln, pcr);
		proj_list_dxlnode->AddChild(pdxlnPrEl);
	}
	
	// cleanup
	pcrsOutput->Release();
	
	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjListFromConstTableGet
//
//	@doc:
//		Construct a project list node by creating references to the columns
//		of the given project list of the child node 
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjListFromConstTableGet
	(
	DrgPcr *pdrgpcrReqOutput, 
	DrgPcr *pdrgpcrCTGOutput,
	DrgPdatum *pdrgpdatumValues
	)
{
	GPOS_ASSERT(NULL != pdrgpcrCTGOutput);
	GPOS_ASSERT(NULL != pdrgpdatumValues);
	GPOS_ASSERT(pdrgpcrCTGOutput->Size() == pdrgpdatumValues->Size());
	
	CDXLNode *proj_list_dxlnode = NULL;
	CColRefSet *pcrsOutput = GPOS_NEW(m_memory_pool) CColRefSet(m_memory_pool);
	pcrsOutput->Include(pdrgpcrCTGOutput);

	if (NULL != pdrgpcrReqOutput)
	{
		const ULONG arity = pdrgpcrReqOutput->Size();
		DrgPdatum *pdrgpdatumOrdered = GPOS_NEW(m_memory_pool) DrgPdatum(m_memory_pool);

		for (ULONG ul = 0; ul < arity; ul++)
		{
			CColRef *pcr = (*pdrgpcrReqOutput)[ul];
			ULONG ulPos = UlPosInArray(pcr, pdrgpcrCTGOutput);
			GPOS_ASSERT(ulPos < pdrgpcrCTGOutput->Size());
			IDatum *pdatum = (*pdrgpdatumValues)[ulPos];
			pdatum->AddRef();
			pdrgpdatumOrdered->Append(pdatum);
			pcrsOutput->Exclude(pcr);
		}

		proj_list_dxlnode = PdxlnProjListFromConstTableGet(NULL, pdrgpcrReqOutput, pdrgpdatumOrdered);
		pdrgpdatumOrdered->Release();
	}
	else
	{
		proj_list_dxlnode = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarProjList(m_memory_pool));
	}

	// construct project elements for columns which remained after processing the required list
	CColRefSetIter crsi(*pcrsOutput);
	while (crsi.Advance())
	{
		CColRef *pcr = crsi.Pcr();
		ULONG ulPos = UlPosInArray(pcr, pdrgpcrCTGOutput);
		GPOS_ASSERT(ulPos < pdrgpcrCTGOutput->Size());
		IDatum *pdatum = (*pdrgpdatumValues)[ulPos];
		CDXLScalarConstValue *pdxlopConstValue = pcr->Pmdtype()->PdxlopScConst(m_memory_pool, pdatum);
		CDXLNode *pdxlnPrEl = PdxlnProjElem(pcr, GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopConstValue));
		proj_list_dxlnode->AddChild(pdxlnPrEl);
	}

	// cleanup
	pcrsOutput->Release();

	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjElem
//
//	@doc:
//		 Create a project elem from a given col ref and a value
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjElem
	(
	const CColRef *pcr,
	CDXLNode *pdxlnValue
	)
{
	GPOS_ASSERT(NULL != pcr);
	
	CMDName *mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pcr->Name().Pstr());
	CDXLNode *pdxlnPrEl = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarProjElem(m_memory_pool, pcr->UlId(), mdname));
	
	// attach scalar id expression to proj elem
	pdxlnPrEl->AddChild(pdxlnValue);
	
	return pdxlnPrEl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnProjElem
//
//	@doc:
//		 Create a project elem from a given col ref
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnProjElem
	(
	const CExpression *pexprProjElem
	)
{
	GPOS_ASSERT(NULL != pexprProjElem && 1 == pexprProjElem->Arity());
	
	CScalarProjectElement *popScPrEl = CScalarProjectElement::PopConvert(pexprProjElem->Pop());
	
	CColRef *pcr = popScPrEl->Pcr();
	
	CMDName *mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pcr->Name().Pstr());
	CDXLNode *pdxlnPrEl = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarProjElem(m_memory_pool, pcr->UlId(), mdname));
	
	CExpression *pexprChild = (*pexprProjElem)[0];
	CDXLNode *child_dxlnode = PdxlnScalar(pexprChild);
	
	pdxlnPrEl->AddChild(child_dxlnode);
	
	return pdxlnPrEl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetSortColListDXL
//
//	@doc:
//		 Create a dxl sort column list node from a given order spec
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::GetSortColListDXL
	(
	const COrderSpec *pos
	)
{
	GPOS_ASSERT(NULL != pos);
	
	CDXLNode *sort_col_list_dxl = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSortColList(m_memory_pool));
	
	for (ULONG ul = 0; ul < pos->UlSortColumns(); ul++)
	{
		// get sort column components
		IMDId *pmdidSortOp = pos->PmdidSortOp(ul);
		pmdidSortOp->AddRef();
		
		const CColRef *pcr = pos->Pcr(ul);
		
		COrderSpec::ENullTreatment ent = pos->Ent(ul);
		GPOS_ASSERT(COrderSpec::EntFirst == ent || COrderSpec::EntLast == ent || COrderSpec::EntAuto == ent);
		
		// get sort operator name
		const IMDScalarOp *pmdscop = m_pmda->Pmdscop(pmdidSortOp);
		
		CWStringConst *pstrSortOpName = 
				GPOS_NEW(m_memory_pool) CWStringConst(m_memory_pool, pmdscop->Mdname().GetMDName()->GetBuffer());
		
		BOOL fSortNullsFirst = false;
		if (COrderSpec::EntFirst == ent)
		{
			fSortNullsFirst = true;
		}

		CDXLScalarSortCol *pdxlopSortCol =
				GPOS_NEW(m_memory_pool) CDXLScalarSortCol
							(
							m_memory_pool, 
							pcr->UlId(), 
							pmdidSortOp,
							pstrSortOpName,
							fSortNullsFirst
							);
		
		CDXLNode *pdxlnSortCol = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopSortCol);
		sort_col_list_dxl->AddChild(pdxlnSortCol);
	}
	
	return sort_col_list_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::PdxlnHashExprList
//
//	@doc:
//		 Create a dxl hash expr list node from a given array of column references
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::PdxlnHashExprList
	(
	const DrgPexpr *pdrgpexpr
	)
{
	GPOS_ASSERT(NULL != pdrgpexpr);
	
	CDXLNode *pdxlnHashExprList = 
			GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarHashExprList(m_memory_pool));
	
	for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
	{
		CExpression *pexpr = (*pdrgpexpr)[ul];
		CScalar *popScalar = CScalar::PopConvert(pexpr->Pop());
		IMDId *mdid_type = popScalar->MDIdType();
		mdid_type->AddRef();
	
		// constrct a hash expr node for the col ref
		CDXLNode *pdxlnHashExpr = GPOS_NEW(m_memory_pool) CDXLNode
												(
												m_memory_pool,
												GPOS_NEW(m_memory_pool) CDXLScalarHashExpr
															(
															m_memory_pool,
															mdid_type
															)
												);
												
		pdxlnHashExpr->AddChild(PdxlnScalar(pexpr));
		
		pdxlnHashExprList->AddChild(pdxlnHashExpr);
	}
	
	return pdxlnHashExprList;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetSortColListDXL
//
//	@doc:
//		 Create a dxl sort column list node for a given motion operator
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXL::GetSortColListDXL
	(
	CExpression *pexprMotion
	)
{
	CDXLNode *sort_col_list_dxl = NULL;
	if (COperator::EopPhysicalMotionGather == pexprMotion->Pop()->Eopid())
	{
		// construct a sorting column list node
		CPhysicalMotionGather *popGather = CPhysicalMotionGather::PopConvert(pexprMotion->Pop());
		sort_col_list_dxl = GetSortColListDXL(popGather->Pos());
	}
	else
	{
		sort_col_list_dxl = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarSortColList(m_memory_pool));
	}
	
	return sort_col_list_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetOutputSegIdsArray
//
//	@doc:
//		Construct an array with output segment indices for the given Motion
//		expression.
//
//---------------------------------------------------------------------------
IntPtrArray *
CTranslatorExprToDXL::GetOutputSegIdsArray
	(
	CExpression *pexprMotion
	)
{
	IntPtrArray *pdrgpi = NULL;
	
	COperator *pop = pexprMotion->Pop();
	
	switch (pop->Eopid())
	{
		case COperator::EopPhysicalMotionGather:
		{
			CPhysicalMotionGather *popGather = CPhysicalMotionGather::PopConvert(pop);
		
			pdrgpi = GPOS_NEW(m_memory_pool) IntPtrArray(m_memory_pool);
			INT iSegmentId = m_iMasterId;
			
			if (CDistributionSpecSingleton::EstSegment == popGather->Est())
			{
				// gather to first segment
				iSegmentId = *((*m_pdrgpiSegments)[0]);
			}
			pdrgpi->Append(GPOS_NEW(m_memory_pool) INT(iSegmentId));
			break;
		}
		case COperator::EopPhysicalMotionBroadcast:
		case COperator::EopPhysicalMotionHashDistribute:
		case COperator::EopPhysicalMotionRoutedDistribute:
		case COperator::EopPhysicalMotionRandom:
		{
			m_pdrgpiSegments->AddRef();
			pdrgpi = m_pdrgpiSegments;
			break;
		}
		default:
			GPOS_ASSERT(!"Unrecognized motion operator");
	}
	
	GPOS_ASSERT(NULL != pdrgpi);
	
	return pdrgpi;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::GetInputSegIdsArray
//
//	@doc:
//		Construct an array with input segment indices for the given Motion
//		expression.
//
//---------------------------------------------------------------------------
IntPtrArray *
CTranslatorExprToDXL::GetInputSegIdsArray
	(
	CExpression *pexprMotion
	)
{
	GPOS_ASSERT(1 == pexprMotion->Arity());

	// derive the distribution of child expression
	CExpression *pexprChild = (*pexprMotion)[0];
	CDrvdPropPlan *pdpplan = CDrvdPropPlan::Pdpplan(pexprChild->PdpDerive());
	CDistributionSpec *pds = pdpplan->Pds();

	if (CDistributionSpec::EdtSingleton == pds->Edt() || CDistributionSpec::EdtStrictSingleton == pds->Edt())
	{
		IntPtrArray *pdrgpi = GPOS_NEW(m_memory_pool) IntPtrArray(m_memory_pool);
		INT iSegmentId = m_iMasterId;
		CDistributionSpecSingleton *pdss = CDistributionSpecSingleton::PdssConvert(pds);
		if (!pdss->FOnMaster())
		{
			// non-master singleton is currently fixed to the first segment
			iSegmentId = *((*m_pdrgpiSegments)[0]);
		}
		pdrgpi->Append(GPOS_NEW(m_memory_pool) INT(iSegmentId));
		return pdrgpi;
	}

	if (CUtils::FDuplicateHazardMotion(pexprMotion))
	{
		// if Motion is duplicate-hazard, we have to read from one input segment
		// to avoid generating duplicate values
		IntPtrArray *pdrgpi = GPOS_NEW(m_memory_pool) IntPtrArray(m_memory_pool);
		INT iSegmentId = *((*m_pdrgpiSegments)[0]);
		pdrgpi->Append(GPOS_NEW(m_memory_pool) INT(iSegmentId));
		return pdrgpi;
	}

	m_pdrgpiSegments->AddRef();
	return m_pdrgpiSegments;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXL::UlPosInArray
//
//	@doc:
//		Find position of colref in the array
//
//---------------------------------------------------------------------------
ULONG
CTranslatorExprToDXL::UlPosInArray
	(
	const CColRef *pcr,
	const DrgPcr *pdrgpcr
	)
	const
{
	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(NULL != pcr);
	
	const ULONG ulSize = pdrgpcr->Size();
	
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		if (pcr == (*pdrgpcr)[ul])
		{
			return ul;
		}
	}
	
	// not found
	return ulSize;
}

// A wrapper around CTranslatorExprToDXLUtils::PdxlnResult to check if the project list imposes a motion hazard,
// eventually leading to a deadlock. If yes, add a Materialize on the Result child to break the deadlock cycle
CDXLNode *
CTranslatorExprToDXL::PdxlnResult
	(
	CDXLPhysicalProperties *dxl_properties,
	CDXLNode *pdxlnPrL,
	CDXLNode *child_dxlnode
	)
{
	CDXLNode *pdxlnMaterialize = NULL;
	CDXLNode *pdxlnScalarOneTimeFilter = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOneTimeFilter(m_memory_pool));

	// If the result project list contains a subplan with a Broadcast motion,
	// along with other projections from the result's child node with a motion as well,
	// it may result in a deadlock. In such cases, add a materialize node.
	if (FNeedsMaterializeUnderResult(pdxlnPrL, child_dxlnode))
	{
		pdxlnMaterialize = PdxlnMaterialize(child_dxlnode);
	}

	return CTranslatorExprToDXLUtils::PdxlnResult
										(
										 m_memory_pool,
										 dxl_properties,
										 pdxlnPrL,
										 PdxlnFilter(NULL),
										 pdxlnScalarOneTimeFilter,
										 pdxlnMaterialize ? pdxlnMaterialize: child_dxlnode
										 );
}

CDXLNode *
CTranslatorExprToDXL::PdxlnMaterialize
	(
	CDXLNode *pdxln // node that needs to be materialized
	)
{
	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(NULL != pdxln->GetProperties());

	CDXLPhysicalMaterialize *pdxlopMaterialize = GPOS_NEW(m_memory_pool) CDXLPhysicalMaterialize(m_memory_pool, true /* fEager */);
	CDXLNode *pdxlnMaterialize = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopMaterialize);
	CDXLPhysicalProperties *pdxlpropChild = CDXLPhysicalProperties::PdxlpropConvert(pdxln->GetProperties());
	pdxlpropChild->AddRef();
	pdxlnMaterialize->SetProperties(pdxlpropChild);

	// construct an empty filter node
	CDXLNode *filter_dxlnode = PdxlnFilter(NULL /* pdxlnCond */);

	CDXLNode *pdxlnProjListChild = (*pdxln)[0];
	CDXLNode *proj_list_dxlnode = CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(m_memory_pool, m_pcf, m_phmcrdxln, pdxlnProjListChild);

	// add children
	pdxlnMaterialize->AddChild(proj_list_dxlnode);
	pdxlnMaterialize->AddChild(filter_dxlnode);
	pdxlnMaterialize->AddChild(pdxln);
	return pdxlnMaterialize;
}

BOOL
CTranslatorExprToDXL::FNeedsMaterializeUnderResult
	(
	CDXLNode *proj_list_dxlnode,
	CDXLNode *child_dxlnode
	)
{
	BOOL fMotionHazard = false;

	// if there is no subplan with a broadcast motion in the project list,
	// then don't bother checking for motion hazard
	BOOL fPrjListContainsSubplan = CTranslatorExprToDXLUtils::FProjListContainsSubplanWithBroadCast(proj_list_dxlnode);

	if (fPrjListContainsSubplan)
	{
		CBitSet *pbsScIdentColIds = GPOS_NEW(m_memory_pool) CBitSet(m_memory_pool);

		// recurse into project elements to extract out columns ids of scalar idents
		CTranslatorExprToDXLUtils::ExtractIdentColIds(proj_list_dxlnode, pbsScIdentColIds);

		// result node will impose motion hazard only if it projects a Subplan
		// and an Ident produced by a tree that contains a motion
		if (pbsScIdentColIds->Size() > 0)
		{
			// motions which can impose a hazard
			gpdxl::Edxlopid rgeopid[] = {
				EdxlopPhysicalMotionBroadcast,
				EdxlopPhysicalMotionRedistribute,
				EdxlopPhysicalMotionRandom,
			};

			fMotionHazard = CTranslatorExprToDXLUtils::FMotionHazard(
																	 m_memory_pool,
																	 child_dxlnode,
																	 rgeopid,
																	 GPOS_ARRAY_SIZE(rgeopid),
																	 pbsScIdentColIds);
		}
		pbsScIdentColIds->Release();
	}
	return fMotionHazard;
}
// EOF
