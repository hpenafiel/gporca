//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CTranslatorExprToDXLUtils.cpp
//
//	@doc:
//		Implementation of the helper methods used during Expr to DXL translation
//		
//---------------------------------------------------------------------------

#include "gpopt/translate/CTranslatorExprToDXLUtils.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "naucrates/md/IMDCast.h"

#include "gpopt/exception.h"

#include "naucrates/dxl/operators/dxlops.h"
#include "naucrates/dxl/operators/CDXLDatumInt4.h"
#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/operators/CDXLDatumOid.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"

#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDTypeOid.h"

#include "naucrates/statistics/IStatistics.h"

#include "gpopt/base/CConstraint.h"
#include "gpopt/base/CConstraintConjunction.h"
#include "gpopt/base/CConstraintDisjunction.h"
#include "gpopt/base/CConstraintNegation.h"
#include "gpopt/base/CConstraintInterval.h"

using namespace gpos;
using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnInt4Const
//
//	@doc:
// 		Construct a scalar const value expression for the given INT value
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnInt4Const
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	INT iVal
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	const IMDTypeInt4 *pmdtypeint4 = pmda->PtMDType<IMDTypeInt4>();
	pmdtypeint4->MDId()->AddRef();
	
	CDXLDatumInt4 *datum_dxl = GPOS_NEW(memory_pool) CDXLDatumInt4(memory_pool, pmdtypeint4->MDId(), false /*is_null*/, iVal);
	CDXLScalarConstValue *pdxlConst = GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, datum_dxl);
	
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlConst);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnBoolConst
//
//	@doc:
// 		Construct a scalar const value expression for the given BOOL value
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnBoolConst
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	BOOL value
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	const IMDTypeBool *pmdtype = pmda->PtMDType<IMDTypeBool>();
	pmdtype->MDId()->AddRef();
	
	CDXLDatumBool *datum_dxl = GPOS_NEW(memory_pool) CDXLDatumBool(memory_pool, pmdtype->MDId(), false /*is_null*/, value);
	CDXLScalarConstValue *pdxlConst = GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, datum_dxl);
	
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlConst);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTest
//
//	@doc:
// 		Construct a test expression for the given part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTest
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	const CPartConstraint *ppartcnstr,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	CharPtrArray *pdrgszPartTypes
	)
{	
	DXLNodeArray *dxl_array = GPOS_NEW(memory_pool) DXLNodeArray(memory_pool);

	const ULONG ulLevels = pdrgpdrgpcrPartKeys->Size();
	for (ULONG ul = 0; ul < ulLevels; ul++)
	{
		CConstraint *pcnstr = ppartcnstr->Pcnstr(ul);
		DrgDrgPcr *pdrgpdrgpcr = ppartcnstr->Pdrgpdrgpcr();
		BOOL fRangePart = (IMDRelation::ErelpartitionRange == *(*pdrgszPartTypes)[ul]);
		CDXLNode *pdxlnPartialScanTest = PdxlnPartialScanTest(memory_pool, pmda, pcf, pcnstr, pdrgpdrgpcr, fRangePart);

		// check whether the scalar filter is of the form "where false"
		BOOL fScalarFalse = FScalarConstFalse(pmda, pdxlnPartialScanTest);
		if (!fScalarFalse)
		{
			// add (AND not defaultpart) to the previous condition
			CDXLNode *pdxlnNotDefault = GPOS_NEW(memory_pool) CDXLNode
											(
											memory_pool,
											GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlnot),
											PdxlnDefaultPartitionTest(memory_pool, ul)
											);

			pdxlnPartialScanTest = GPOS_NEW(memory_pool) CDXLNode
											(
											memory_pool,
											GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland),
											pdxlnNotDefault,
											pdxlnPartialScanTest
											);
		}

		if (ppartcnstr->FDefaultPartition(ul))
		{
			CDXLNode *pdxlnDefaultPartitionTest = PdxlnDefaultPartitionTest(memory_pool, ul);

			if (fScalarFalse)
			{
				pdxlnPartialScanTest->Release();
				pdxlnPartialScanTest = pdxlnDefaultPartitionTest;
			}
			else
			{
				pdxlnPartialScanTest = GPOS_NEW(memory_pool) CDXLNode
										(
										memory_pool,
										GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor),
										pdxlnPartialScanTest,
										pdxlnDefaultPartitionTest
										);
			}
		}

		dxl_array->Append(pdxlnPartialScanTest);
	}

	if (1 == dxl_array->Size())
	{
		CDXLNode *pdxln = (*dxl_array)[0];
		pdxln->AddRef();
		dxl_array->Release();
		return pdxln;
	}

	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland), dxl_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnDefaultPartitionTest
//
//	@doc:
// 		Construct a test expression for default partitions
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnDefaultPartitionTest
	(
	IMemoryPool *memory_pool, 
	ULONG ulPartLevel
	)
{	
	CDXLNode *pdxlnDefaultPart = GPOS_NEW(memory_pool) CDXLNode
									(
									memory_pool,
									GPOS_NEW(memory_pool) CDXLScalarPartDefault(memory_pool, ulPartLevel)
									);

	return pdxlnDefaultPart;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTest
//
//	@doc:
// 		Construct a test expression for the given part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTest
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	CConstraint *pcnstr,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	BOOL fRangePart
	)
{
	GPOS_ASSERT(NULL != pcnstr);
	
	if (pcnstr->FContradiction())
	{
		return PdxlnBoolConst(memory_pool, pmda, false /*value*/);
	}
	
	switch (pcnstr->Ect())
	{
		case CConstraint::EctConjunction:
			return PdxlnPartialScanTestConjunction(memory_pool, pmda, pcf, pcnstr, pdrgpdrgpcrPartKeys, fRangePart);
			
		case CConstraint::EctDisjunction:
			return PdxlnPartialScanTestDisjunction(memory_pool, pmda, pcf, pcnstr, pdrgpdrgpcrPartKeys, fRangePart);

		case CConstraint::EctNegation:
			return PdxlnPartialScanTestNegation(memory_pool, pmda, pcf, pcnstr, pdrgpdrgpcrPartKeys, fRangePart);

		case CConstraint::EctInterval:
			return PdxlnPartialScanTestInterval(memory_pool, pmda, pcnstr, pdrgpdrgpcrPartKeys, fRangePart);

		default:
			GPOS_RAISE
				(
				gpdxl::ExmaDXL,
				gpdxl::ExmiExpr2DXLUnsupportedFeature,
				GPOS_WSZ_LIT("Unrecognized constraint type")
				);
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTestConjDisj
//
//	@doc:
// 		Construct a test expression for the given conjunction or disjunction 
//		based part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTestConjDisj
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	DrgPcnstr *pdrgpcnstr,
	BOOL fConjunction,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	BOOL fRangePart
	)
{	
	GPOS_ASSERT(NULL != pdrgpcnstr);
	
	const ULONG length = pdrgpcnstr->Size();
	
	if (1 == length)
	{
		return PdxlnPartialScanTest(memory_pool, pmda, pcf, (*pdrgpcnstr)[0], pdrgpdrgpcrPartKeys, fRangePart);
	}
	
	EdxlBoolExprType edxlbooltype = Edxlor;
	
	if (fConjunction)
	{
		edxlbooltype = Edxland;
	}
	
	CDXLNode *pdxlnResult = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, edxlbooltype));
	
	for (ULONG ul = 0; ul < length; ul++)
	{
		CConstraint *pcnstr = (*pdrgpcnstr)[ul];
		CDXLNode *pdxln = PdxlnPartialScanTest(memory_pool, pmda, pcf, pcnstr, pdrgpdrgpcrPartKeys, fRangePart);
		pdxlnResult->AddChild(pdxln);
	}
	
	return pdxlnResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPropagationExpressionForPartConstraints
//
//	@doc:
// 		Construct a nested if statement testing the constraints in the 
//		given part index map and propagating to the right part index id
//		
//		For example for the following part constraint map:
//		1->[1,3), 2->[3,5), 3->![1,5), the generated if expr will be:
//		If (min,max,minincl,maxincl) \subseteq [1,3)
//		Then 1
//		Else If (min,max,minincl,maxincl) \subseteq [3,5)
//		     Then 2
//		     Else If (min,max,minincl,maxincl) \subseteq Not([1,5))
//		          Then 3
//		          Else NULL
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPropagationExpressionForPartConstraints
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	PartCnstrMap *ppartcnstrmap,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	CharPtrArray *pdrgszPartTypes
	)
{	
	PartCnstrMapIter pcmi(ppartcnstrmap);
		
	CDXLNode *pdxlnScalarRootIfStmt = NULL;
	CDXLNode *pdxlnScalarLeafIfStmt = NULL;
	
	const IMDTypeInt4 *pmdtypeint4 = pmda->PtMDType<IMDTypeInt4>();
	IMDId *pmdidRetType = pmdtypeint4->MDId();

	while (pcmi.Advance())
	{
		ULONG ulSecondaryScanId = *(pcmi.Key());
		const CPartConstraint *ppartcnstr = pcmi.Value();
		CDXLNode *pdxlnTest = PdxlnPartialScanTest
									(
									memory_pool, 
									pmda, 
									pcf,
									ppartcnstr,
									pdrgpdrgpcrPartKeys,
									pdrgszPartTypes
									);
		
		CDXLNode *pdxlnPropagate = PdxlnInt4Const(memory_pool, pmda, (INT) ulSecondaryScanId);
		
		pmdidRetType->AddRef();
		CDXLNode *pdxlnScalarIf = GPOS_NEW(memory_pool) CDXLNode
										(
										memory_pool, 
										GPOS_NEW(memory_pool) CDXLScalarIfStmt(memory_pool, pmdidRetType),
										pdxlnTest, 
										pdxlnPropagate
										);
		
		if (NULL == pdxlnScalarRootIfStmt)
		{
			pdxlnScalarRootIfStmt = pdxlnScalarIf;
		}
		else
		{
			// add nested if statement to the latest leaf if statement as the else case of the already constructed if stmt
			GPOS_ASSERT(NULL != pdxlnScalarLeafIfStmt && 2 == pdxlnScalarLeafIfStmt->Arity());
			pdxlnScalarLeafIfStmt->AddChild(pdxlnScalarIf);
		}
		
		pdxlnScalarLeafIfStmt = pdxlnScalarIf;
	}
	
	GPOS_ASSERT(2 == pdxlnScalarLeafIfStmt->Arity());
	
	// add a dummy value for the top and bottom level else cases
	const IMDType *pmdtypeVoid = pmda->Pmdtype(pmdidRetType);
	CDXLDatum *datum_dxl = pmdtypeVoid->PdxldatumNull(memory_pool);
	CDXLNode *pdxlnNullConst = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, datum_dxl));
	pdxlnScalarLeafIfStmt->AddChild(pdxlnNullConst);
	
	if (2 == pdxlnScalarRootIfStmt->Arity())
	{
		pdxlnNullConst->AddRef();
		pdxlnScalarRootIfStmt->AddChild(pdxlnNullConst);
	}
	
	return pdxlnScalarRootIfStmt;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTestConjunction
//
//	@doc:
// 		Construct a test expression for the given conjunction  
//		based part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTestConjunction
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	CConstraint *pcnstr,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	BOOL fRangePart
	)
{	
	GPOS_ASSERT(CConstraint::EctConjunction == pcnstr->Ect());
	
	CConstraintConjunction *pcnstrConj = dynamic_cast<CConstraintConjunction *>(pcnstr);
	
	DrgPcnstr *pdrgpcnstr = pcnstrConj->Pdrgpcnstr();
	return PdxlnPartialScanTestConjDisj(memory_pool, pmda, pcf, pdrgpcnstr, true /*fConjunction*/, pdrgpdrgpcrPartKeys, fRangePart);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTestDisjunction
//
//	@doc:
// 		Construct a test expression for the given disjunction  
//		based part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTestDisjunction
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	CConstraint *pcnstr,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	BOOL fRangePart
	)
{	
	GPOS_ASSERT(CConstraint::EctDisjunction == pcnstr->Ect());
	
	CConstraintDisjunction *pcnstrDisj = dynamic_cast<CConstraintDisjunction *>(pcnstr);
	
	DrgPcnstr *pdrgpcnstr = pcnstrDisj->Pdrgpcnstr();
	return PdxlnPartialScanTestConjDisj(memory_pool, pmda, pcf, pdrgpcnstr, false /*fConjunction*/, pdrgpdrgpcrPartKeys, fRangePart);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTestNegation
//
//	@doc:
// 		Construct a test expression for the given negation  
//		based part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTestNegation
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CColumnFactory *pcf,
	CConstraint *pcnstr,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	BOOL fRangePart
	)
{	
	GPOS_ASSERT(CConstraint::EctNegation == pcnstr->Ect());
	
	CConstraintNegation *pcnstrNeg = dynamic_cast<CConstraintNegation *>(pcnstr);
	
	CConstraint *pcnstrChild = pcnstrNeg->PcnstrChild();

	CDXLNode *child_dxlnode = PdxlnPartialScanTest(memory_pool, pmda, pcf, pcnstrChild, pdrgpdrgpcrPartKeys, fRangePart);
	
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlnot), child_dxlnode);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTestInterval
//
//	@doc:
// 		Construct a test expression for the given interval  
//		based part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTestInterval
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CConstraint *pcnstr,
	DrgDrgPcr *pdrgpdrgpcrPartKeys,
	BOOL fRangePart
	)
{	
	GPOS_ASSERT(CConstraint::EctInterval == pcnstr->Ect());
	
	CConstraintInterval *pcnstrInterval = dynamic_cast<CConstraintInterval *>(pcnstr);
	
	const CColRef *pcrPartKey = pcnstrInterval->Pcr();
	IMDId *pmdidPartKeyType = pcrPartKey->Pmdtype()->MDId();
	ULONG ulPartLevel = UlPartKeyLevel(pcrPartKey, pdrgpdrgpcrPartKeys);

	DrgPrng *pdrgprng = pcnstrInterval->Pdrgprng();
	const ULONG ulRanges = pdrgprng->Size();
	 
	GPOS_ASSERT(0 < ulRanges);
	
	if (1 == ulRanges)
	{
		return PdxlnPartialScanTestRange(memory_pool, pmda, (*pdrgprng)[0], pmdidPartKeyType, ulPartLevel, fRangePart);
	}
	
	CDXLNode *pdxlnDisjunction = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor));
	
	for (ULONG ul = 0; ul < ulRanges; ul++)
	{
		CRange *prng = (*pdrgprng)[ul];
		CDXLNode *child_dxlnode = PdxlnPartialScanTestRange(memory_pool, pmda, prng, pmdidPartKeyType, ulPartLevel, fRangePart);
		pdxlnDisjunction->AddChild(child_dxlnode);
	}
	
	return pdxlnDisjunction;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::UlPartKeyLevel
//
//	@doc:
// 		Find the partitioning level of the given part key, given the whole
//		array of part keys
//
//---------------------------------------------------------------------------
ULONG
CTranslatorExprToDXLUtils::UlPartKeyLevel
	(
	const CColRef *pcr,
	DrgDrgPcr *pdrgpdrgpcr
	)
{
	GPOS_ASSERT(0 < pdrgpdrgpcr->Size() && "No partitioning keys found");

	const ULONG length = pdrgpdrgpcr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		if (CUtils::PcrExtractPartKey(pdrgpdrgpcr, ul) == pcr)
		{
			return ul;
		}
	}

	GPOS_ASSERT(!"Could not find partitioning key");
	return 0;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartialScanTestRange
//
//	@doc:
// 		Construct a test expression for the given range or list
//		based part constraint
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartialScanTestRange
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CRange *prng,
	IMDId *pmdidPartKeyType,
	ULONG ulPartLevel,
	BOOL fRangePart
	)
{	
	if (fRangePart)
	{
		CDXLNode *pdxlnStart = PdxlnRangeStartPredicate(memory_pool, pmda, prng->PdatumLeft(), prng->EriLeft(), pmdidPartKeyType, ulPartLevel);
		CDXLNode *pdxlnEnd = PdxlnRangeEndPredicate(memory_pool, pmda, prng->PdatumRight(), prng->EriRight(), pmdidPartKeyType, ulPartLevel);

		return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland), pdxlnStart, pdxlnEnd);
	}
	else // list partition
	{
		IDatum *pdatum = prng->PdatumLeft();
		if (pdatum == NULL)
		{
			// TODO: In case of default partitions, we end up with NULL for Left and Right Datum.
			// Currently we fallback and should handle it better in future.
			GPOS_RAISE
				(
				gpdxl::ExmaDXL,
				gpdxl::ExmiExpr2DXLUnsupportedFeature,
				GPOS_WSZ_LIT("Queries over default list partition that have indexes")
				);
		}
		GPOS_ASSERT(pdatum->FMatch(prng->PdatumRight()));

		CDXLDatum *datum_dxl = Pdxldatum(memory_pool, pmda, pdatum);
		CDXLNode *pdxlnScalar = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, datum_dxl));
		// TODO: what if part key type is varchar, the value type is text?
		const IMDType *pmdtype = pmda->Pmdtype(pmdidPartKeyType);
		IMDId *pmdidResult = pmdtype->PmdidTypeArray();
		pmdidResult->AddRef();
		pmdidPartKeyType->AddRef();
		CDXLNode *pdxlnPartList = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartListValues(memory_pool, ulPartLevel, pmdidResult, pmdidPartKeyType));

		IMDId *pmdidEq = pmdtype->PmdidCmp(IMDType::EcmptEq);
		pmdidEq->AddRef();
		CDXLNode *pdxlnScCmp = GPOS_NEW(memory_pool) CDXLNode
													(
													memory_pool,
													GPOS_NEW(memory_pool) CDXLScalarArrayComp
																(
																memory_pool,
																pmdidEq,
																GPOS_NEW(memory_pool) CWStringConst(memory_pool, pmdidEq->GetBuffer()),
																Edxlarraycomptypeany
																),
													pdxlnScalar,
													pdxlnPartList
													);
		return pdxlnScCmp;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeStartPredicate
//
//	@doc:
// 		Construct a test expression for the given range start point
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangeStartPredicate
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	IDatum *pdatum,
	CRange::ERangeInclusion eri,
	IMDId *pmdidPartKeyType,
	ULONG ulPartLevel
	)
{	
	const IMDType *pmdtype = pmda->Pmdtype(pmdidPartKeyType);
	
	return PdxlnRangePointPredicate
			(
			memory_pool, 
			pmda, 
			pdatum,
			eri, 
			pmdidPartKeyType, 
			pmdtype->PmdidCmp(IMDType::EcmptL), 	// pmdidCmpExl
			pmdtype->PmdidCmp(IMDType::EcmptLEq), 	// pmdidCmpIncl
			ulPartLevel,
			true /*fLower*/
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeEndPredicate
//
//	@doc:
// 		Construct a test expression for the given range end point
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangeEndPredicate
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	IDatum *pdatum,
	CRange::ERangeInclusion eri,
	IMDId *pmdidPartKeyType,
	ULONG ulPartLevel
	)
{	
	const IMDType *pmdtype = pmda->Pmdtype(pmdidPartKeyType);
	
	return PdxlnRangePointPredicate
			(
			memory_pool, 
			pmda, 
			pdatum,
			eri, 
			pmdidPartKeyType, 
			pmdtype->PmdidCmp(IMDType::EcmptG), 	// pmdidCmpExl
			pmdtype->PmdidCmp(IMDType::EcmptGEq), 	// pmdidCmpIncl
			ulPartLevel,
			false /*fLower*/
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangePointPredicate
//
//	@doc:
// 		Construct a test expression for the given range point using the 
//		provided comparison operators
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangePointPredicate
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	IDatum *pdatum,
	CRange::ERangeInclusion eri,
	IMDId *pmdidPartKeyType,
	IMDId *pmdidCmpExl,
	IMDId *pmdidCmpIncl,
	ULONG ulPartLevel,
	BOOL fLower
	)
{	
	if (NULL == pdatum)
	{
		// point in an unbounded range: create a predicate (open-ended)
		return GPOS_NEW(memory_pool) CDXLNode
					(
					memory_pool,
					GPOS_NEW(memory_pool) CDXLScalarPartBoundOpen(memory_pool, ulPartLevel, fLower)
					);
	}
	
	pmdidPartKeyType->AddRef();
	CDXLNode *pdxlnPartBound = GPOS_NEW(memory_pool) CDXLNode
										(
										memory_pool,
										GPOS_NEW(memory_pool) CDXLScalarPartBound(memory_pool, ulPartLevel, pmdidPartKeyType, fLower)
										);

	CDXLDatum *datum_dxl = Pdxldatum(memory_pool, pmda, pdatum);
	CDXLNode *pdxlnPoint = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, datum_dxl));
	 	
	// generate a predicate of the form "point < col" / "point > col"
	pmdidCmpExl->AddRef();
	
	CWStringConst *pstrCmpExcl = GPOS_NEW(memory_pool) CWStringConst(memory_pool, pmda->Pmdscop(pmdidCmpExl)->Mdname().Pstr()->GetBuffer());
	CDXLNode *pdxlnPredicateExclusive = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarComp(memory_pool, pmdidCmpExl, pstrCmpExcl), pdxlnPoint, pdxlnPartBound);
	
	// generate a predicate of the form "point <= col and colIncluded" / "point >= col and colIncluded"
	pmdidCmpIncl->AddRef();

	CWStringConst *pstrCmpIncl = GPOS_NEW(memory_pool) CWStringConst(memory_pool, pmda->Pmdscop(pmdidCmpIncl)->Mdname().Pstr()->GetBuffer());
	pdxlnPartBound->AddRef();
	pdxlnPoint->AddRef();
	CDXLNode *pdxlnCmpIncl = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarComp(memory_pool, pmdidCmpIncl, pstrCmpIncl), pdxlnPoint, pdxlnPartBound);

	CDXLNode *pdxlnPartBoundInclusion = GPOS_NEW(memory_pool) CDXLNode
										(
										memory_pool,
										GPOS_NEW(memory_pool) CDXLScalarPartBoundInclusion(memory_pool, ulPartLevel, fLower)
										);

	if (CRange::EriExcluded == eri)
	{
		// negate the "inclusion" portion of the predicate
		pdxlnPartBoundInclusion = GPOS_NEW(memory_pool) CDXLNode
											(
											memory_pool,
											GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlnot),
											pdxlnPartBoundInclusion
											);
	}

	CDXLNode *pdxlnPredicateInclusive = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland), pdxlnCmpIncl, pdxlnPartBoundInclusion);

	// return the final predicate in the form "(point <= col and colInclusive) or point < col" / "(point >= col and colInclusive) or point > col"
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor), pdxlnPredicateInclusive, pdxlnPredicateExclusive);
}


// construct a DXL node for the part key portion of the list partition filter
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnListFilterPartKey
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CExpression *pexprPartKey,
	IMDId *pmdidTypePartKey,
	ULONG ulPartLevel
	)
{
	GPOS_ASSERT(NULL != pexprPartKey);
	GPOS_ASSERT(NULL != pmdidTypePartKey);
	GPOS_ASSERT(CScalar::PopConvert(pexprPartKey->Pop())->MDIdType()->Equals(pmdidTypePartKey));

	CDXLNode *pdxlnPartKey = NULL;

	if (CUtils::FScalarIdent(pexprPartKey))
	{
		// Simple Scalar Ident - create a ScalarPartListValues from the partition key
		IMDId *pmdidResultArray = pmda->Pmdtype(pmdidTypePartKey)->PmdidTypeArray();
		pmdidResultArray->AddRef();
		pmdidTypePartKey->AddRef();

		pdxlnPartKey = GPOS_NEW(memory_pool) CDXLNode
						(
						memory_pool,
						GPOS_NEW(memory_pool) CDXLScalarPartListValues
								(
								memory_pool,
								ulPartLevel,
								pmdidResultArray,
								pmdidTypePartKey
								)
						);
	}
	else if (CScalarIdent::FCastedScId(pexprPartKey))
	{
		// ScalarCast(ScalarIdent) - create an ArrayCoerceExpr over a ScalarPartListValues
		CScalarCast *pexprScalarCast = CScalarCast::PopConvert(pexprPartKey->Pop());
		IMDId *pmdidDestElem = pexprScalarCast->MDIdType();
		IMDId *pmdidDestArray = pmda->Pmdtype(pmdidDestElem)->PmdidTypeArray();

		CScalarIdent *pexprScalarIdent = CScalarIdent::PopConvert((*pexprPartKey)[0]->Pop());
		IMDId *pmdidSrcElem = pexprScalarIdent->MDIdType();
		IMDId *pmdidSrcArray = pmda->Pmdtype(pmdidSrcElem)->PmdidTypeArray();

		IMDId *pmdidArrayCastFunc = NULL;

		if (CMDAccessorUtils::FCastExists(pmda, pmdidSrcElem, pmdidDestElem))
		{
			const IMDCast *pmdcast = pmda->Pmdcast(pmdidSrcElem, pmdidDestElem);
			pmdidArrayCastFunc = pmdcast->PmdidCastFunc();
		}

		pmdidSrcArray->AddRef();
		pmdidSrcElem->AddRef();
		CDXLNode *pdxlnPartKeyIdent = GPOS_NEW(memory_pool) CDXLNode
							(
							memory_pool,
							GPOS_NEW(memory_pool) CDXLScalarPartListValues
									(
									memory_pool,
									ulPartLevel,
									pmdidSrcArray,
									pmdidSrcElem
									)
							);

		pmdidDestArray->AddRef();
		pmdidArrayCastFunc->AddRef();
		pdxlnPartKey = GPOS_NEW(memory_pool) CDXLNode
					(
					memory_pool,
					GPOS_NEW(memory_pool) CDXLScalarArrayCoerceExpr
									(
									memory_pool,
									pmdidArrayCastFunc,
									pmdidDestArray,
									IDefaultTypeModifier,
									true, /* fIsExplicit */
									EdxlcfDontCare,
									-1 /* iLoc */
									),
					pdxlnPartKeyIdent
					);
	}
	else
	{
		// Not supported - should be unreachable.
		CWStringDynamic *pstr = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);
		pstr->AppendFormat(GPOS_WSZ_LIT("Unsupported part filter operator for list partitions : %ls"),
						   pexprPartKey->Pop()->SzId());
		GPOS_THROW_EXCEPTION(gpopt::ExmaGPOPT,
							 gpopt::ExmiUnsupportedOp,
							 CException::ExsevDebug1,
							 pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != pdxlnPartKey);

	return pdxlnPartKey;
}


// Construct a predicate node for a list partition filter
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnListFilterScCmp
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CDXLNode *pdxlnPartKey,
	CDXLNode *pdxlnOther,
	IMDId *pmdidTypePartKey,
	IMDId *pmdidTypeOther,
	IMDType::ECmpType ecmpt,
	ULONG ulPartLevel,
	BOOL fHasDefaultPart
	)
{
	IMDId *pmdidScCmp = NULL;

	pmdidScCmp = CUtils::PmdidScCmp(memory_pool, pmda, pmdidTypeOther, pmdidTypePartKey, ecmpt);

	const IMDScalarOp *pmdscop = pmda->Pmdscop(pmdidScCmp);
	const CWStringConst *pstrScCmp = pmdscop->Mdname().Pstr();

	pmdidScCmp->AddRef();
	CDXLNode *pdxlnScCmp = GPOS_NEW(memory_pool) CDXLNode
												(
												memory_pool,
												GPOS_NEW(memory_pool) CDXLScalarArrayComp
															(
															memory_pool,
															pmdidScCmp,
															GPOS_NEW(memory_pool) CWStringConst(memory_pool, pstrScCmp->GetBuffer()),
															Edxlarraycomptypeany
															),
												pdxlnOther,
												pdxlnPartKey
												);

	if (fHasDefaultPart)
	{
		CDXLNode *pdxlnDefault = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartDefault(memory_pool, ulPartLevel));
		return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor), pdxlnScCmp, pdxlnDefault);
	}
	else
	{
		return pdxlnScCmp;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterScCmp
//
//	@doc:
// 		Construct a Result node for a filter min <= Scalar or max >= Scalar
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangeFilterScCmp
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CDXLNode *pdxlnScalar,
	IMDId *pmdidTypePartKey,
	IMDId *pmdidTypeOther,
	IMDId *pmdidTypeCastExpr,
	IMDId *pmdidCastFunc,
	IMDType::ECmpType ecmpt,
	ULONG ulPartLevel
	)
{
	if (IMDType::EcmptEq == ecmpt)
	{
		return PdxlnRangeFilterEqCmp
				(
				memory_pool, 
				pmda, 
				pdxlnScalar, 
				pmdidTypePartKey, 
				pmdidTypeOther,
				pmdidTypeCastExpr,
				pmdidCastFunc,
				ulPartLevel
				);
	}
	
	BOOL fLowerBound = false;
	IMDType::ECmpType ecmptScCmp = IMDType::EcmptOther;
	
	if (IMDType::EcmptLEq == ecmpt || IMDType::EcmptL == ecmpt)
	{
		// partkey </<= other: construct condition min < other
		fLowerBound = true;
		ecmptScCmp = IMDType::EcmptL;
	}
	else 
	{
		GPOS_ASSERT(IMDType::EcmptGEq == ecmpt || IMDType::EcmptG == ecmpt);
		
		// partkey >/>= other: construct condition max > other
		ecmptScCmp = IMDType::EcmptG;
	}
	
	CDXLNode *pdxlnPredicateExclusive = PdxlnCmp(memory_pool, pmda, ulPartLevel, fLowerBound, pdxlnScalar, ecmptScCmp, pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, pmdidCastFunc);
	
	if (IMDType::EcmptLEq != ecmpt && IMDType::EcmptGEq != ecmpt)
	{
		// scalar comparison does not include equality: no need to consider part constraint boundaries
		return pdxlnPredicateExclusive;
	}
	
	pdxlnScalar->AddRef();
	CDXLNode *pdxlnInclusiveCmp = PdxlnCmp(memory_pool, pmda, ulPartLevel, fLowerBound, pdxlnScalar, ecmpt, pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, pmdidCastFunc);
	CDXLNode *pdxlnInclusiveBoolPredicate = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartBoundInclusion(memory_pool, ulPartLevel, fLowerBound));

	CDXLNode *pdxlnPredicateInclusive = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland), pdxlnInclusiveCmp, pdxlnInclusiveBoolPredicate);
	
	// return the final predicate in the form "(point <= col and colIncluded) or point < col" / "(point >= col and colIncluded) or point > col"
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor), pdxlnPredicateInclusive, pdxlnPredicateExclusive);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterEqCmp
//
//	@doc:
// 		Construct a predicate node for a filter min <= Scalar and max >= Scalar
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangeFilterEqCmp
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CDXLNode *pdxlnScalar,
	IMDId *pmdidTypePartKey,
	IMDId *pmdidTypeOther,
	IMDId *pmdidTypeCastExpr,
	IMDId *pmdidCastFunc,
	ULONG ulPartLevel
	)
{
	CDXLNode *pdxlnPredicateMin = PdxlnRangeFilterPartBound(memory_pool, pmda, pdxlnScalar, pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, pmdidCastFunc, ulPartLevel, true /*fLowerBound*/, IMDType::EcmptL);
	pdxlnScalar->AddRef();
	CDXLNode *pdxlnPredicateMax = PdxlnRangeFilterPartBound(memory_pool, pmda, pdxlnScalar, pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, pmdidCastFunc, ulPartLevel, false /*fLowerBound*/, IMDType::EcmptG);
		
	// return the conjunction of the predicate for the lower and upper bounds
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland), pdxlnPredicateMin, pdxlnPredicateMax);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterPartBound
//
//	@doc:
// 		Construct a predicate for a partition bound of one of the two forms 
//		(min <= Scalar and minincl) or min < Scalar
//		(max >= Scalar and maxinc) or Max > Scalar
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangeFilterPartBound
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda, 
	CDXLNode *pdxlnScalar,
	IMDId *pmdidTypePartKey,
	IMDId *pmdidTypeOther,
	IMDId *pmdidTypeCastExpr,
	IMDId *pmdidCastFunc,
	ULONG ulPartLevel,
	ULONG fLowerBound,
	IMDType::ECmpType ecmpt
	)
{
	GPOS_ASSERT(IMDType::EcmptL == ecmpt || IMDType::EcmptG == ecmpt);
	
	IMDType::ECmpType ecmptInc = IMDType::EcmptLEq;
	if (IMDType::EcmptG == ecmpt)
	{
		ecmptInc = IMDType::EcmptGEq;
	}

	CDXLNode *pdxlnPredicateExclusive = PdxlnCmp(memory_pool, pmda, ulPartLevel, fLowerBound, pdxlnScalar, ecmpt, pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, pmdidCastFunc);

	pdxlnScalar->AddRef();
	CDXLNode *pdxlnInclusiveCmp = PdxlnCmp(memory_pool, pmda, ulPartLevel, fLowerBound, pdxlnScalar, ecmptInc, pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, pmdidCastFunc);
	
	CDXLNode *pdxlnInclusiveBoolPredicate = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartBoundInclusion(memory_pool, ulPartLevel, fLowerBound));
	
	CDXLNode *pdxlnPredicateInclusive = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxland), pdxlnInclusiveCmp, pdxlnInclusiveBoolPredicate);
	
	// return the final predicate in the form "(point <= col and colIncluded) or point < col" / "(point >= col and colIncluded) or point > col"
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor), pdxlnPredicateInclusive, pdxlnPredicateExclusive);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterDefaultAndOpenEnded
//
//	@doc:
//		Construct predicates to cover the cases of default partition and
//		open-ended partitions if necessary
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnRangeFilterDefaultAndOpenEnded
	(
	IMemoryPool *memory_pool, 
	ULONG ulPartLevel,
	BOOL fLTComparison,
	BOOL fGTComparison,
	BOOL fEQComparison,
	BOOL fDefaultPart
	)
{
	CDXLNode *pdxlnResult = NULL;
	if (fLTComparison || fEQComparison)
	{
		// add a condition to cover the cases of open-ended interval (-inf, x)
		pdxlnResult = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartBoundOpen(memory_pool, ulPartLevel, true /*fLower*/));
	}
	
	if (fGTComparison || fEQComparison)
	{
		// add a condition to cover the cases of open-ended interval (x, inf)
		CDXLNode *pdxlnOpenMax = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartBoundOpen(memory_pool, ulPartLevel, false /*fLower*/));

		// construct a boolean OR expression over the two expressions
		if (NULL != pdxlnResult)
		{
			pdxlnResult = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor), pdxlnResult, pdxlnOpenMax);
		}
		else
		{
			pdxlnResult = pdxlnOpenMax;
		}
	}

	if (fDefaultPart)
	{
		// add a condition to cover the cases of default partition
		CDXLNode *pdxlnDefault = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartDefault(memory_pool, ulPartLevel));

		if (NULL != pdxlnResult)
		{
			pdxlnResult = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, Edxlor), pdxlnDefault, pdxlnResult);
		}
		else
		{
			pdxlnResult = pdxlnDefault;
		}

	}
	
	return pdxlnResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlpropCopy
//
//	@doc:
//		Return a copy the dxl node's physical properties
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties *
CTranslatorExprToDXLUtils::PdxlpropCopy
	(
	IMemoryPool *memory_pool,
	CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);

	GPOS_ASSERT(NULL != pdxln->GetProperties());
	CDXLPhysicalProperties *dxl_properties = CDXLPhysicalProperties::PdxlpropConvert(pdxln->GetProperties());

	CWStringDynamic *pstrStartupcost = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, dxl_properties->GetOperatorCostDXL()->GetStartUpCostStr()->GetBuffer());
	CWStringDynamic *pstrCost = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, dxl_properties->GetOperatorCostDXL()->GetTotalCostStr()->GetBuffer());
	CWStringDynamic *rows_out_str = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, dxl_properties->GetOperatorCostDXL()->GetRowsOutStr()->GetBuffer());
	CWStringDynamic *width_str = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, dxl_properties->GetOperatorCostDXL()->GetWidthStr()->GetBuffer());

	return GPOS_NEW(memory_pool) CDXLPhysicalProperties(GPOS_NEW(memory_pool) CDXLOperatorCost(pstrStartupcost, pstrCost, rows_out_str, width_str));
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnCmp
//
//	@doc:
//		Construct a scalar comparison of the given type between the column with
//		the given col id and the scalar expression
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnCmp
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda,
	ULONG ulPartLevel,
	BOOL fLowerBound,
	CDXLNode *pdxlnScalar, 
	IMDType::ECmpType ecmpt, 
	IMDId *pmdidTypePartKey,
	IMDId *pmdidTypeExpr,
	IMDId *pmdidTypeCastExpr,
	IMDId *pmdidCastFunc
	)
{
	IMDId *pmdidScCmp = NULL;

	if (IMDId::IsValid(pmdidTypeCastExpr))
	{
		pmdidScCmp = CUtils::PmdidScCmp(memory_pool, pmda, pmdidTypeCastExpr, pmdidTypeExpr, ecmpt);
	}
	else
	{
		pmdidScCmp = CUtils::PmdidScCmp(memory_pool, pmda, pmdidTypePartKey, pmdidTypeExpr, ecmpt);
	}
	
	const IMDScalarOp *pmdscop = pmda->Pmdscop(pmdidScCmp); 
	const CWStringConst *pstrScCmp = pmdscop->Mdname().Pstr();
	
	pmdidScCmp->AddRef();
	
	CDXLScalarComp *pdxlopCmp = GPOS_NEW(memory_pool) CDXLScalarComp(memory_pool, pmdidScCmp, GPOS_NEW(memory_pool) CWStringConst(memory_pool, pstrScCmp->GetBuffer()));
	CDXLNode *pdxlnScCmp = GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlopCmp);
	
	pmdidTypePartKey->AddRef();
	CDXLNode *pdxlnPartBound = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartBound(memory_pool, ulPartLevel, pmdidTypePartKey, fLowerBound));
	
	if (IMDId::IsValid(pmdidTypeCastExpr))
	{
		GPOS_ASSERT(NULL != pmdidCastFunc);
		pmdidTypeCastExpr->AddRef();
		pmdidCastFunc->AddRef();

		pdxlnPartBound = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarCast(memory_pool, pmdidTypeCastExpr, pmdidCastFunc), pdxlnPartBound);
	}
	pdxlnScCmp->AddChild(pdxlnPartBound);
	pdxlnScCmp->AddChild(pdxlnScalar);
	
	return pdxlnScCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PcrCreate
//
//	@doc:
//		Construct a column reference with the given name and type
//
//---------------------------------------------------------------------------
CColRef *
CTranslatorExprToDXLUtils::PcrCreate
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CColumnFactory *pcf,
	IMDId *mdid_type,
	INT type_modifier,
	const WCHAR *wszName
	)
{
	const IMDType *pmdtype = pmda->Pmdtype(mdid_type);
	
	CName *pname = GPOS_NEW(memory_pool) CName(GPOS_NEW(memory_pool) CWStringConst(wszName), true /*fOwnsMemory*/);
	CColRef *pcr = pcf->PcrCreate(pmdtype, type_modifier, *pname);
	GPOS_DELETE(pname);
	return pcr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::GetProperties
//
//	@doc:
//		Construct a DXL physical properties container with operator costs for
//		the given expression
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties *
CTranslatorExprToDXLUtils::GetProperties
	(
	IMemoryPool *memory_pool
	)
{
	// TODO:  - May 10, 2012; replace the dummy implementation with a real one
	CWStringDynamic *pstrStartupcost = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("10"));
	CWStringDynamic *pstrTotalcost = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("100"));
	CWStringDynamic *rows_out_str = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("100"));
	CWStringDynamic *width_str = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool, GPOS_WSZ_LIT("4"));

	CDXLOperatorCost *pdxlopcost = GPOS_NEW(memory_pool) CDXLOperatorCost(pstrStartupcost, pstrTotalcost, rows_out_str, width_str);
	return GPOS_NEW(memory_pool) CDXLPhysicalProperties(pdxlopcost);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FScalarConstTrue
//
//	@doc:
//		Checks to see if the DXL Node is a scalar const TRUE
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FScalarConstTrue
	(
	CMDAccessor *pmda,
	CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	if (EdxlopScalarConstValue == pdxln->GetOperator()->GetDXLOperator())
	{
		CDXLScalarConstValue *pdxlopConst =
				CDXLScalarConstValue::Cast(pdxln->GetOperator());

		const IMDType *pmdtype = pmda->Pmdtype(pdxlopConst->Pdxldatum()->MDId());
		if (IMDType::EtiBool ==  pmdtype->Eti())
		{
			CDXLDatumBool *datum_dxl = CDXLDatumBool::Cast(const_cast<CDXLDatum*>(pdxlopConst->Pdxldatum()));

			return (!datum_dxl->IsNull() && datum_dxl->FValue());
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FScalarConstFalse
//
//	@doc:
//		Checks to see if the DXL Node is a scalar const FALSE
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FScalarConstFalse
	(
	CMDAccessor *pmda,
	CDXLNode *pdxln
	)
{
	GPOS_ASSERT(NULL != pdxln);
	if (EdxlopScalarConstValue == pdxln->GetOperator()->GetDXLOperator())
	{
		CDXLScalarConstValue *pdxlopConst =
				CDXLScalarConstValue::Cast(pdxln->GetOperator());

		const IMDType *pmdtype = pmda->Pmdtype(pdxlopConst->Pdxldatum()->MDId());
		if (IMDType::EtiBool ==  pmdtype->Eti())
		{
			CDXLDatumBool *datum_dxl = CDXLDatumBool::Cast(const_cast<CDXLDatum*>(pdxlopConst->Pdxldatum()));
			return (!datum_dxl->IsNull() && !datum_dxl->FValue());
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList
//
//	@doc:
//		Construct a project list node by creating references to the columns
//		of the given project list of the child node 
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	HMCrDxln *phmcrdxln, 
	const CDXLNode *pdxlnProjListChild
	)
{
	GPOS_ASSERT(NULL != pdxlnProjListChild);
	
	CDXLScalarProjList *pdxlopPrL = GPOS_NEW(memory_pool) CDXLScalarProjList(memory_pool);
	CDXLNode *pdxlnProjList = GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlopPrL);
	
	// create a scalar identifier for each project element of the child
	const ULONG ulArity = pdxlnProjListChild->Arity();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLNode *pdxlnProjElemChild = (*pdxlnProjListChild)[ul];
		
		// translate proj elem
		CDXLNode *pdxlnProjElem = PdxlnProjElem(memory_pool, pcf, phmcrdxln, pdxlnProjElemChild);
		pdxlnProjList->AddChild(pdxlnProjElem);
	}
		
	return pdxlnProjList;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector
//
//	@doc:
//		Construct the project list of a partition selector
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CColumnFactory *pcf,
	HMCrDxln *phmcrdxln,
	BOOL fUseChildProjList,
	CDXLNode *pdxlnPrLChild,
	CColRef *pcrOid,
	ULONG ulPartLevels,
	BOOL fGeneratePartOid
	)
{
	GPOS_ASSERT_IMP(fUseChildProjList, NULL != pdxlnPrLChild);

	CDXLNode *pdxlnPrL = NULL;
	if (fUseChildProjList)
	{
		pdxlnPrL = PdxlnProjListFromChildProjList(memory_pool, pcf, phmcrdxln, pdxlnPrLChild);
	}
	else
	{
		pdxlnPrL = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarProjList(memory_pool));
	}

	if (fGeneratePartOid)
	{
		// add to it the Oid column
		if (NULL == pcrOid)
		{
			const IMDTypeOid *pmdtype = pmda->PtMDType<IMDTypeOid>();
			pcrOid = pcf->PcrCreate(pmdtype, IDefaultTypeModifier);
		}

		CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pcrOid->Name().Pstr());
		CDXLScalarProjElem *pdxlopPrEl = GPOS_NEW(memory_pool) CDXLScalarProjElem(memory_pool, pcrOid->UlId(), mdname);
		CDXLNode *pdxlnPrEl = GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlopPrEl);
		CDXLNode *pdxlnPartOid = GPOS_NEW(memory_pool) CDXLNode(memory_pool, GPOS_NEW(memory_pool) CDXLScalarPartOid(memory_pool, ulPartLevels-1));
		pdxlnPrEl->AddChild(pdxlnPartOid);
		pdxlnPrL->AddChild(pdxlnPrEl);
	}

	return pdxlnPrL;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPropExprPartitionSelector
//
//	@doc:
//		Construct the propagation expression for a partition selector
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPropExprPartitionSelector
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CColumnFactory *pcf,
	BOOL fConditional,
	PartCnstrMap *ppartcnstrmap,
	DrgDrgPcr *pdrgpdrgpcrKeys,
	ULONG ulScanId,
	CharPtrArray *pdrgszPartTypes
	)
{
	if (!fConditional)
	{
		// unconditional propagation
		return PdxlnInt4Const(memory_pool, pmda, (INT) ulScanId);
	}

	return PdxlnPropagationExpressionForPartConstraints(memory_pool, pmda, pcf, ppartcnstrmap, pdrgpdrgpcrKeys, pdrgszPartTypes);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnProjElem
//
//	@doc:
//		Create a project elem as a scalar identifier for the given child 
//		project element
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnProjElem
	(
	IMemoryPool *memory_pool,
	CColumnFactory *pcf,
	HMCrDxln *phmcrdxln, 
	const CDXLNode *pdxlnChildProjElem
	)
{
	GPOS_ASSERT(NULL != pdxlnChildProjElem && 1 == pdxlnChildProjElem->Arity());
	
	CDXLScalarProjElem *pdxlopPrElChild = dynamic_cast<CDXLScalarProjElem*>(pdxlnChildProjElem->GetOperator());

    // find the col ref corresponding to this element's id through column factory
    CColRef *pcr = pcf->PcrLookup(pdxlopPrElChild->UlId());
    if (NULL == pcr)
    {
    	GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiExpr2DXLAttributeNotFound, pdxlopPrElChild->UlId());
    }
    
    CDXLNode *pdxlnProjElemResult = PdxlnProjElem(memory_pool, phmcrdxln, pcr);
	
	return pdxlnProjElemResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::ReplaceSubplan
//
//	@doc:
//		 Replace subplan entry in the given map with a dxl column ref
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::ReplaceSubplan
	(
	IMemoryPool *memory_pool,
	HMCrDxln *phmcrdxlnSubplans,  // map of col ref to subplan
	const CColRef *pcr, // key of entry in the passed map
	CDXLScalarProjElem *pdxlopPrEl // project element to use for creating DXL col ref to replace subplan
	)
{
	GPOS_ASSERT(NULL != phmcrdxlnSubplans);
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(pdxlopPrEl->UlId() == pcr->UlId());

	IMDId *mdid_type = pcr->Pmdtype()->MDId();
	mdid_type->AddRef();
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pdxlopPrEl->PmdnameAlias()->Pstr());
	CDXLColRef *pdxlcr = GPOS_NEW(memory_pool) CDXLColRef(memory_pool, mdname, pdxlopPrEl->UlId(), mdid_type, pcr->TypeModifier());
	CDXLScalarIdent *pdxlnScId = GPOS_NEW(memory_pool) CDXLScalarIdent(memory_pool, pdxlcr);
	CDXLNode *pdxln = GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlnScId);
#ifdef GPOS_DEBUG
	BOOL fReplaced =
#endif // GPOS_DEBUG
		phmcrdxlnSubplans->Replace(pcr, pdxln);
	GPOS_ASSERT(fReplaced);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnProjElem
//
//	@doc:
//		Create a project elem from a given col ref,
//
//		if the given col has a corresponding subplan entry in the given map,
//		the function returns a project element with a child subplan,
//		the function then replaces the subplan entry in the given map with the
//		projected column reference, so that all subplan references higher up in
//		the DXL tree use the projected col ref
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnProjElem
	(
	IMemoryPool *memory_pool,
	HMCrDxln *phmcrdxlnSubplans, // map of col ref -> subplan: can be modified by this function
	const CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pcr);
	
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pcr->Name().Pstr());
	
	CDXLScalarProjElem *pdxlopPrEl = GPOS_NEW(memory_pool) CDXLScalarProjElem(memory_pool, pcr->UlId(), mdname);
	CDXLNode *pdxlnPrEl = GPOS_NEW(memory_pool) CDXLNode(memory_pool, pdxlopPrEl);
	
	// create a scalar identifier for the proj element expression
	CDXLNode *pdxlnScId = PdxlnIdent(memory_pool, phmcrdxlnSubplans, NULL /*phmcrdxlnIndexLookup*/, pcr);

	if (EdxlopScalarSubPlan == pdxlnScId->GetOperator()->GetDXLOperator())
	{
		// modify map by replacing subplan entry with the projected
		// column reference so that all subplan references higher up
		// in the DXL tree use the projected col ref
		ReplaceSubplan(memory_pool, phmcrdxlnSubplans, pcr, pdxlopPrEl);
	}

	// attach scalar id expression to proj elem
	pdxlnPrEl->AddChild(pdxlnScId);
	
	return pdxlnPrEl;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnIdent
//
//	@doc:
//		 Create a scalar identifier node from a given col ref
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnIdent
	(
	IMemoryPool *memory_pool,
	HMCrDxln *phmcrdxlnSubplans,
	HMCrDxln *phmcrdxlnIndexLookup,
	const CColRef *pcr
	)
{
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(NULL != phmcrdxlnSubplans);
	
	CDXLNode *pdxln = phmcrdxlnSubplans->Find(pcr);

	if (NULL != pdxln)
	{
		pdxln->AddRef();
		return pdxln;
	}

	if (NULL != phmcrdxlnIndexLookup)
	{
		CDXLNode *pdxlnIdent = phmcrdxlnIndexLookup->Find(pcr);
		if (NULL != pdxlnIdent)
		{
			pdxlnIdent->AddRef();
			return pdxlnIdent;
		}
	}

	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pcr->Name().Pstr());

	IMDId *pmdid = pcr->Pmdtype()->MDId();
	pmdid->AddRef();

	CDXLColRef *pdxlcr = GPOS_NEW(memory_pool) CDXLColRef(memory_pool, mdname, pcr->UlId(), pmdid, pcr->TypeModifier());
	
	CDXLScalarIdent *dxl_op = GPOS_NEW(memory_pool) CDXLScalarIdent(memory_pool, pdxlcr);
	return GPOS_NEW(memory_pool) CDXLNode(memory_pool, dxl_op);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdrgpdatumNulls
//
//	@doc:
//		Create an array of NULL datums for a given array of columns
//
//---------------------------------------------------------------------------
DrgPdatum *
CTranslatorExprToDXLUtils::PdrgpdatumNulls
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr
	)
{
	DrgPdatum *pdrgpdatum = GPOS_NEW(memory_pool) DrgPdatum(memory_pool);

	const ULONG ulSize = pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		const IMDType *pmdtype = pcr->Pmdtype();
		IDatum *pdatum = pmdtype->PdatumNull();
		pdatum->AddRef();
		pdrgpdatum->Append(pdatum);
	}

	return pdrgpdatum;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FProjectListMatch
//
//	@doc:
//		Check whether a project list has the same columns in the given array
//		and in the same order
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FProjectListMatch
	(
	CDXLNode *pdxlnPrL,
	DrgPcr *pdrgpcr
	)
{
	GPOS_ASSERT(NULL != pdxlnPrL);
	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());

	const ULONG ulLen = pdrgpcr->Size();
	if (pdxlnPrL->Arity() != ulLen)
	{
		return false;
	}

	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];

		CDXLNode *pdxlnPrEl = (*pdxlnPrL)[ul];
		CDXLScalarProjElem *pdxlopPrEl = CDXLScalarProjElem::Cast(pdxlnPrEl->GetOperator());

		if (pcr->UlId() != pdxlopPrEl->UlId())
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdrgpcrMapColumns
//
//	@doc:
//		Map an array of columns to a new array of columns. The column index is
//		look up in the given hash map, then the corresponding column from
//		the destination array is used
//
//---------------------------------------------------------------------------
DrgPcr *
CTranslatorExprToDXLUtils::PdrgpcrMapColumns
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrInput,
	HMCrUl *phmcrul,
	DrgPcr *pdrgpcrMapDest
	)
{
	GPOS_ASSERT(NULL != phmcrul);
	GPOS_ASSERT(NULL != pdrgpcrMapDest);

	if (NULL == pdrgpcrInput)
	{
		return NULL;
	}

	DrgPcr *pdrgpcrNew = GPOS_NEW(memory_pool) DrgPcr(memory_pool);

	const ULONG ulLen = pdrgpcrInput->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CColRef *pcr = (*pdrgpcrInput)[ul];

		// get column index from hashmap
		ULONG *pul = phmcrul->Find(pcr);
		GPOS_ASSERT (NULL != pul);

		// add corresponding column from dest array
		pdrgpcrNew->Append((*pdrgpcrMapDest)[*pul]);
	}

	return pdrgpcrNew;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnResult
//
//	@doc:
//		Create a DXL result node using the given properties, project list,
//		filters, and relational child
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnResult
	(
	IMemoryPool *memory_pool,
	CDXLPhysicalProperties *dxl_properties,
	CDXLNode *pdxlnPrL,
	CDXLNode *pdxlnFilter,
	CDXLNode *pdxlnOneTimeFilter,
	CDXLNode *child_dxlnode
	)
{
	CDXLPhysicalResult *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalResult(memory_pool);
	CDXLNode *pdxlnResult = GPOS_NEW(memory_pool) CDXLNode(memory_pool, dxl_op);
	pdxlnResult->SetProperties(dxl_properties);
	
	pdxlnResult->AddChild(pdxlnPrL);
	pdxlnResult->AddChild(pdxlnFilter);
	pdxlnResult->AddChild(pdxlnOneTimeFilter);

	if (NULL != child_dxlnode)
	{
		pdxlnResult->AddChild(child_dxlnode);
	}

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(pdxlnResult, false /* validate_children */);
#endif

	return pdxlnResult;
}

// create a DXL Value Scan node
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnValuesScan
	(
	IMemoryPool *memory_pool,
	CDXLPhysicalProperties *dxl_properties,
	CDXLNode *pdxlnPrL,
	DrgPdrgPdatum *pdrgpdrgdatum
	)
{
	CDXLPhysicalValuesScan *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalValuesScan(memory_pool);
	CDXLNode *pdxlnValuesScan = GPOS_NEW(memory_pool) CDXLNode(memory_pool, dxl_op);
	pdxlnValuesScan->SetProperties(dxl_properties);

	pdxlnValuesScan->AddChild(pdxlnPrL);

	const ULONG ulTuples = pdrgpdrgdatum->Size();

	for (ULONG ulTuplePos = 0; ulTuplePos < ulTuples; ulTuplePos++)
	{
		DrgPdatum *pdrgpdatum = (*pdrgpdrgdatum)[ulTuplePos];
		pdrgpdatum->AddRef();
		const ULONG ulCols = pdrgpdatum->Size();
		CDXLScalarValuesList *values = GPOS_NEW(memory_pool) CDXLScalarValuesList(memory_pool);
		CDXLNode *pdxlnValueList = GPOS_NEW(memory_pool) CDXLNode(memory_pool, values);

		for (ULONG ulColPos = 0; ulColPos < ulCols; ulColPos++)
		{
			IDatum *pdatum = (*pdrgpdatum)[ulColPos];
			CMDAccessor *pmda = COptCtxt::PoctxtFromTLS()->Pmda();
			const IMDType *pmdtype = pmda->Pmdtype(pdatum->MDId());

			CDXLNode *pdxlnValue = GPOS_NEW(memory_pool) CDXLNode(memory_pool, pmdtype->PdxlopScConst(memory_pool, pdatum));
			pdxlnValueList->AddChild(pdxlnValue);
		}
		pdrgpdatum->Release();
		pdxlnValuesScan->AddChild(pdxlnValueList);
	}

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(pdxlnValuesScan, true /* validate_children */);
#endif

	return pdxlnValuesScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPartitionSelector
//
//	@doc:
//		Construct a partition selector node
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnPartitionSelector
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	ULONG ulPartLevels,
	ULONG ulScanId,
	CDXLPhysicalProperties *dxl_properties,
	CDXLNode *pdxlnPrL,
	CDXLNode *pdxlnEqFilters,
	CDXLNode *pdxlnFilters,
	CDXLNode *pdxlnResidual,
	CDXLNode *pdxlnPropagation,
	CDXLNode *pdxlnPrintable,
	CDXLNode *child_dxlnode
	)
{
	CDXLNode *pdxlnSelector = GPOS_NEW(memory_pool) CDXLNode
										(
										memory_pool,
										GPOS_NEW(memory_pool) CDXLPhysicalPartitionSelector(memory_pool, pmdid, ulPartLevels, ulScanId)
										);

	pdxlnSelector->SetProperties(dxl_properties);
	pdxlnSelector->AddChild(pdxlnPrL);
	pdxlnSelector->AddChild(pdxlnEqFilters);
	pdxlnSelector->AddChild(pdxlnFilters);
	pdxlnSelector->AddChild(pdxlnResidual);
	pdxlnSelector->AddChild(pdxlnPropagation);
	pdxlnSelector->AddChild(pdxlnPrintable);
	if (NULL != child_dxlnode)
	{
		pdxlnSelector->AddChild(child_dxlnode);
	}

	return pdxlnSelector;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnCombineBoolean
//
//	@doc:
//		Combine two boolean expressions using the given boolean operator
//
//---------------------------------------------------------------------------
CDXLNode *
CTranslatorExprToDXLUtils::PdxlnCombineBoolean
	(
	IMemoryPool *memory_pool,
	CDXLNode *first_child_dxlnode,
	CDXLNode *second_child_dxlnode,
	EdxlBoolExprType boolexptype
	)
{
	GPOS_ASSERT(Edxlor == boolexptype ||
				Edxland == boolexptype);

	if (NULL == first_child_dxlnode)
	{
		return second_child_dxlnode;
	}

	if (NULL == second_child_dxlnode)
	{
		return first_child_dxlnode;
	}

	return  GPOS_NEW(memory_pool) CDXLNode
						(
						memory_pool,
						GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, boolexptype),
						first_child_dxlnode,
						second_child_dxlnode
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PhmcrulColIndex
//
//	@doc:
//		Build a hashmap based on a column array, where the key is the column
//		and the value is the index of that column in the array
//
//---------------------------------------------------------------------------
HMCrUl *
CTranslatorExprToDXLUtils::PhmcrulColIndex
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr
	)
{
	HMCrUl *phmcrul = GPOS_NEW(memory_pool) HMCrUl(memory_pool);

	const ULONG ulLen = pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CColRef *pcr = (*pdrgpcr)[ul];
		ULONG *pul = GPOS_NEW(memory_pool) ULONG(ul);

		// add to hashmap
	#ifdef GPOS_DEBUG
		BOOL fRes =
	#endif // GPOS_DEBUG
		phmcrul->Insert(pcr, pul);
		GPOS_ASSERT(fRes);
	}

	return phmcrul;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::SetStats
//
//	@doc:
//		Set the statistics of the dxl operator
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::SetStats
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CDXLNode *pdxln,
	const IStatistics *pstats,
	BOOL fRoot
	)
{
	if (NULL != pstats && GPOS_FTRACE(EopttraceExtractDXLStats) && 
		(GPOS_FTRACE(EopttraceExtractDXLStatsAllNodes) || fRoot)
		)
	{
		CDXLPhysicalProperties::PdxlpropConvert(pdxln->GetProperties())->SetStats(pstats->Pdxlstatsderrel(memory_pool, pmda));
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::SetDirectDispatchInfo
//
//	@doc:
//		Set the direct dispatch info of the dxl operator
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::SetDirectDispatchInfo
	(
	IMemoryPool *memory_pool,
	CMDAccessor *pmda,
	CDXLNode *pdxln,
	CDrvdPropRelational *pdpRel,
	DrgPds *pdrgpdsBaseTables
	)
{
	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(NULL != pdpRel);
	GPOS_ASSERT(NULL != pdrgpdsBaseTables);
	
	Edxlopid edxlopid = pdxln->GetOperator()->GetDXLOperator();
	if (EdxlopPhysicalCTAS == edxlopid || EdxlopPhysicalDML == edxlopid || EdxlopPhysicalRowTrigger == edxlopid)
	{
		// direct dispatch for CTAS and DML handled elsewhere
		// TODO:  - Oct 15, 2014; unify
		return;
	}
	
	if (1 != pdpRel->UlJoinDepth() || 1 != pdrgpdsBaseTables->Size())
	{
		// direct dispatch not supported for join queries
		return;
	}
		
	CDistributionSpec *pds = (*pdrgpdsBaseTables)[0];
	
	if (CDistributionSpec::EdtHashed != pds->Edt())
	{
		// direct dispatch only supported for scans over hash distributed tables 
		return;
	}
	
	CPropConstraint *ppc = pdpRel->Ppc();
	if (NULL == ppc->Pcnstr())
	{
		return;
	}
		
	GPOS_ASSERT(NULL != ppc->Pcnstr());
	
	CDistributionSpecHashed *pdsHashed = CDistributionSpecHashed::PdsConvert(pds);
	DrgPexpr *pdrgpexprHashed = pdsHashed->Pdrgpexpr();
	
	CDXLDirectDispatchInfo *dxl_direct_dispatch_info = GetDXLDirectDispatchInfo(memory_pool, pmda, pdrgpexprHashed, ppc->Pcnstr());
	
	if (NULL != dxl_direct_dispatch_info)
	{
		pdxln->SetDirectDispatchInfo(dxl_direct_dispatch_info);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::GetDXLDirectDispatchInfo
//
//	@doc:
//		Compute the direct dispatch info spec. Returns NULL if this is not
//		possible
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo *
CTranslatorExprToDXLUtils::GetDXLDirectDispatchInfo
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda,
	DrgPexpr *pdrgpexprHashed, 
	CConstraint *pcnstr
	)
{
	GPOS_ASSERT(NULL != pdrgpexprHashed);
	GPOS_ASSERT(NULL != pcnstr);
	
	const ULONG ulHashExpr = pdrgpexprHashed->Size();
	GPOS_ASSERT(0 < ulHashExpr);
	
	if (1 == ulHashExpr)
	{
		CExpression *pexprHashed = (*pdrgpexprHashed)[0];
		return PdxlddinfoSingleDistrKey(memory_pool, pmda, pexprHashed, pcnstr);
	}
	
	BOOL fSuccess = true;
	DXLDatumArray *pdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArray(memory_pool);

	for (ULONG ul = 0; ul < ulHashExpr && fSuccess; ul++)
	{
		CExpression *pexpr = (*pdrgpexprHashed)[ul];
		if (!CUtils::FScalarIdent(pexpr))
		{
			fSuccess = false; 
			break;
		}
		
		const CColRef *pcrDistrCol = CScalarIdent::PopConvert(pexpr->Pop())->Pcr();
		
		CConstraint *pcnstrDistrCol = pcnstr->Pcnstr(memory_pool, pcrDistrCol);
		
		CDXLDatum *datum_dxl = PdxldatumFromPointConstraint(memory_pool, pmda, pcrDistrCol, pcnstrDistrCol);
		CRefCount::SafeRelease(pcnstrDistrCol);

		if (NULL != datum_dxl && FDirectDispatchable(pcrDistrCol, datum_dxl))
		{
			pdrgpdxldatum->Append(datum_dxl);
		}
		else
		{
			CRefCount::SafeRelease(datum_dxl);

			fSuccess = false;
			break;
		}
	}
	
	if (!fSuccess)
	{
		pdrgpdxldatum->Release();

		return NULL;
	}
	
	DXLDatumArrays *pdrgpdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArrays(memory_pool);
	pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
	return GPOS_NEW(memory_pool) CDXLDirectDispatchInfo(pdrgpdrgpdxldatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlddinfoSingleDistrKey
//
//	@doc:
//		Compute the direct dispatch info spec for a single distribution key.
//		Returns NULL if this is not possible
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo *
CTranslatorExprToDXLUtils::PdxlddinfoSingleDistrKey
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda,
	CExpression *pexprHashed, 
	CConstraint *pcnstr
	)
{
	GPOS_ASSERT(NULL != pexprHashed);
	if (!CUtils::FScalarIdent(pexprHashed))
	{
		return NULL;
	}
	
	const CColRef *pcrDistrCol = CScalarIdent::PopConvert(pexprHashed->Pop())->Pcr();
	
	CConstraint *pcnstrDistrCol = pcnstr->Pcnstr(memory_pool, pcrDistrCol);
	
	DXLDatumArrays *pdrgpdrgpdxldatum = NULL;
	
	if (CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		CDXLDatum *datum_dxl = PdxldatumFromPointConstraint(memory_pool, pmda, pcrDistrCol, pcnstrDistrCol);
		GPOS_ASSERT(NULL != datum_dxl);

		if (FDirectDispatchable(pcrDistrCol, datum_dxl))
		{
			DXLDatumArray *pdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArray(memory_pool);

			datum_dxl->AddRef();
			pdrgpdxldatum->Append(datum_dxl);
		
			pdrgpdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArrays(memory_pool);
			pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
		}

		datum_dxl->Release();
	}
	else if (CPredicateUtils::FColumnDisjunctionOfConst(pcnstrDistrCol, pcrDistrCol))
	{
		pdrgpdrgpdxldatum = PdrgpdrgpdxldatumFromDisjPointConstraint(memory_pool, pmda, pcrDistrCol, pcnstrDistrCol);
	}
	
	CRefCount::SafeRelease(pcnstrDistrCol);

	if (NULL == pdrgpdrgpdxldatum)
	{
		return NULL;
	}
	
	return GPOS_NEW(memory_pool) CDXLDirectDispatchInfo(pdrgpdrgpdxldatum);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FDirectDispatchable
//
//	@doc:
//		Check if the given constant value for a particular distribution column
// 		can be used to identify which segment to direct dispatch to.
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FDirectDispatchable
	(
	const CColRef *pcrDistrCol,
	const CDXLDatum *datum_dxl
	)
{
	GPOS_ASSERT(NULL != pcrDistrCol);
	GPOS_ASSERT(NULL != datum_dxl);

	IMDId *pmdidDatum = datum_dxl->MDId();
	IMDId *pmdidDistrCol = pcrDistrCol->Pmdtype()->MDId();

	// since all integer values are up-casted to int64, the hash value will be
	// consistent. If either the constant or the distribution column are
	// not integers, then their datatypes must be identical to ensure that
	// the hash value of the constant will point to the right segment.
	BOOL fBothInt = CUtils::FIntType(pmdidDistrCol) && CUtils::FIntType(pmdidDatum);

	return fBothInt || (pmdidDatum->Equals(pmdidDistrCol));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxldatumFromPointConstraint
//
//	@doc:
//		Compute a DXL datum from a point constraint. Returns NULL if this is not
//		possible
//
//---------------------------------------------------------------------------
CDXLDatum *
CTranslatorExprToDXLUtils::PdxldatumFromPointConstraint
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda,
	const CColRef *pcrDistrCol, 
	CConstraint *pcnstrDistrCol
	)
{
	if (!CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		return NULL;
	}
	
	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());
	
	CConstraintInterval *pci = dynamic_cast<CConstraintInterval *>(pcnstrDistrCol);
	GPOS_ASSERT(1 >= pci->Pdrgprng()->Size());
	
	CDXLDatum *datum_dxl = NULL;
	
	if (1 == pci->Pdrgprng()->Size())
	{
		const CRange *prng = (*pci->Pdrgprng())[0];
		datum_dxl = CTranslatorExprToDXLUtils::Pdxldatum(memory_pool, pmda, prng->PdatumLeft());
	}
	else
	{
		GPOS_ASSERT(pci->FIncludesNull());
		datum_dxl = pcrDistrCol->Pmdtype()->PdxldatumNull(memory_pool);
	}
	
	return datum_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdrgpdrgpdxldatumFromDisjPointConstraint
//
//	@doc:
//		Compute an array of DXL datum arrays from a disjunction of point constraints. 
//		Returns NULL if this is not possible
//
//---------------------------------------------------------------------------
DXLDatumArrays *
CTranslatorExprToDXLUtils::PdrgpdrgpdxldatumFromDisjPointConstraint
	(
	IMemoryPool *memory_pool, 
	CMDAccessor *pmda,
	const CColRef *pcrDistrCol, 
	CConstraint *pcnstrDistrCol
	)
{
	GPOS_ASSERT(NULL != pcnstrDistrCol);
	if (CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		DXLDatumArrays *pdrgpdrgpdxldatum = NULL;

		CDXLDatum *datum_dxl = PdxldatumFromPointConstraint(memory_pool, pmda, pcrDistrCol, pcnstrDistrCol);

		if (FDirectDispatchable(pcrDistrCol, datum_dxl))
		{
			DXLDatumArray *pdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArray(memory_pool);

			datum_dxl->AddRef();
			pdrgpdxldatum->Append(datum_dxl);

			pdrgpdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArrays(memory_pool);
			pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
		}

		// clean up
		datum_dxl->Release();

		return pdrgpdrgpdxldatum;
	}
	
	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());
	
	CConstraintInterval *pcnstrInterval = dynamic_cast<CConstraintInterval *>(pcnstrDistrCol);

	DrgPrng *pdrgprng = pcnstrInterval->Pdrgprng();

	const ULONG ulRanges = pdrgprng->Size();
	DXLDatumArrays *pdrgpdrgpdxdatum = GPOS_NEW(memory_pool) DXLDatumArrays(memory_pool);
	
	for (ULONG ul = 0; ul < ulRanges; ul++)
	{
		CRange *prng = (*pdrgprng)[ul];
		GPOS_ASSERT(prng->FPoint());
		CDXLDatum *datum_dxl = CTranslatorExprToDXLUtils::Pdxldatum(memory_pool, pmda, prng->PdatumLeft());

		if (!FDirectDispatchable(pcrDistrCol, datum_dxl))
		{
			// clean up
			datum_dxl->Release();
			pdrgpdrgpdxdatum->Release();

			return NULL;
		}

		DXLDatumArray *pdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArray(memory_pool);

		pdrgpdxldatum->Append(datum_dxl);
		pdrgpdrgpdxdatum->Append(pdrgpdxldatum);
	}
	
	if (pcnstrInterval->FIncludesNull())
	{
		CDXLDatum *datum_dxl = pcrDistrCol->Pmdtype()->PdxldatumNull(memory_pool);

		if (!FDirectDispatchable(pcrDistrCol, datum_dxl))
		{
			// clean up
			datum_dxl->Release();
			pdrgpdrgpdxdatum->Release();

			return NULL;
		}

		DXLDatumArray *pdrgpdxldatum = GPOS_NEW(memory_pool) DXLDatumArray(memory_pool);
		pdrgpdxldatum->Append(datum_dxl);
		pdrgpdrgpdxdatum->Append(pdrgpdxldatum);
	}
	
	if (0 < pdrgpdrgpdxdatum->Size())
	{
		return pdrgpdrgpdxdatum;
	}

	// clean up
	pdrgpdrgpdxdatum->Release();

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FLocalHashAggStreamSafe
//
//	@doc:
//		Is the aggregate a local hash aggregate that is safe to stream
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FLocalHashAggStreamSafe
	(
	CExpression *pexprAgg
	)
{
	GPOS_ASSERT(NULL != pexprAgg);

	COperator::EOperatorId eopid = pexprAgg->Pop()->Eopid();

	if (COperator::EopPhysicalHashAgg !=  eopid && COperator::EopPhysicalHashAggDeduplicate != eopid)
	{
		// not a hash aggregate
		return false;
	}

	CPhysicalAgg *popAgg = CPhysicalAgg::PopConvert(pexprAgg->Pop());

	// is a local hash aggregate and it generates duplicates (therefore safe to stream)
	return (COperator::EgbaggtypeLocal == popAgg->Egbaggtype()) && popAgg->FGeneratesDuplicates();
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::ExtractCastMdids
//
//	@doc:
//		If operator is a scalar cast, extract cast type and function
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::ExtractCastMdids
	(
	COperator *pop, 
	IMDId **ppmdidType, 
	IMDId **ppmdidCastFunc
	)
{
	GPOS_ASSERT(NULL != pop);
	GPOS_ASSERT(NULL != ppmdidType);
	GPOS_ASSERT(NULL != ppmdidCastFunc);

	if (COperator::EopScalarCast != pop->Eopid())
	{
		// not a cast
		return;
	}

	CScalarCast *popCast = CScalarCast::PopConvert(pop);
	*ppmdidType = popCast->MDIdType();
	*ppmdidCastFunc = popCast->PmdidFunc();
}

BOOL
CTranslatorExprToDXLUtils::FDXLOpExists
	(
	const CDXLOperator *pop,
	const gpdxl::Edxlopid *peopid,
	ULONG ulOps
	)
{
	GPOS_ASSERT(NULL != pop);
	GPOS_ASSERT(NULL != peopid);

	gpdxl::Edxlopid eopid = pop->GetDXLOperator();
	for (ULONG ul = 0; ul < ulOps; ul++)
	{
		if (eopid == peopid[ul])
		{
			return true;
		}
	}

	return false;
}

BOOL
CTranslatorExprToDXLUtils::FHasDXLOp
	(
	const CDXLNode *pdxln,
	const gpdxl::Edxlopid *peopid,
	ULONG ulOps
	)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(NULL != pdxln);
	GPOS_ASSERT(NULL != peopid);

	if (FDXLOpExists(pdxln->GetOperator(), peopid, ulOps))
	{
		return true;
	}

	// recursively check children
	const ULONG ulArity = pdxln->Arity();

	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		if (FHasDXLOp((*pdxln)[ul], peopid, ulOps))
		{
			return true;
		}
	}

	return false;
}

BOOL
CTranslatorExprToDXLUtils::FProjListContainsSubplanWithBroadCast
	(
	CDXLNode *pdxlnPrjList
	)
{
	if (pdxlnPrjList->GetOperator()->GetDXLOperator() == EdxlopScalarSubPlan)
	{
		gpdxl::Edxlopid rgeopidMotion[] =	{
			EdxlopPhysicalMotionBroadcast
		};
		return FHasDXLOp(pdxlnPrjList, rgeopidMotion, GPOS_ARRAY_SIZE(rgeopidMotion));
	}

	const ULONG ulArity = pdxlnPrjList->Arity();

	for (ULONG ul =0; ul < ulArity; ul++)
	{
		if (FProjListContainsSubplanWithBroadCast((*pdxlnPrjList)[ul]))
		{
			return true;
		}
	}

	return false;
}

void
CTranslatorExprToDXLUtils::ExtractIdentColIds
	(
	CDXLNode *pdxln,
	CBitSet *pbs
	)
{
	if (pdxln->GetOperator()->GetDXLOperator() == EdxlopScalarIdent)
	{
		const CDXLColRef *pdxlcr = CDXLScalarIdent::Cast(pdxln->GetOperator())->Pdxlcr();
		pbs->ExchangeSet(pdxlcr->Id());
	}

	ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		ExtractIdentColIds((*pdxln)[ul], pbs);
	}
}

BOOL
CTranslatorExprToDXLUtils::FMotionHazard
	(
	IMemoryPool *memory_pool,
	CDXLNode *pdxln,
	const gpdxl::Edxlopid *peopid,
	ULONG ulOps,
	CBitSet *pbsPrjCols
	)
{
	GPOS_ASSERT(pbsPrjCols);

	// non-streaming operator/Gather motion neutralizes any motion hazard that its subtree imposes
	// hence stop recursing further
	if (FMotionHazardSafeOp(pdxln))
		return false;

	if (FDXLOpExists(pdxln->GetOperator(), peopid, ulOps))
	{
		// check if the current motion node projects any column from the
		// input project list.
		// If yes, then we have detected a motion hazard for the parent Result node.
		CBitSet *pbsPrjList = GPOS_NEW(memory_pool) CBitSet(memory_pool);
		ExtractIdentColIds((*pdxln)[0], pbsPrjList);
		BOOL fDisJoint = pbsPrjCols->IsDisjoint(pbsPrjList);
		pbsPrjList->Release();

		return !fDisJoint;
	}

	// recursively check children
	const ULONG ulArity = pdxln->Arity();

	// In ORCA, inner child of Hash Join is always exhausted first,
	// so only check the outer child for motions
	if (pdxln->GetOperator()->GetDXLOperator() == EdxlopPhysicalHashJoin)
	{
		if (FMotionHazard(memory_pool, (*pdxln)[EdxlhjIndexHashLeft], peopid, ulOps, pbsPrjCols))
		{
			return true;
		}
	}
	else
	{
		for (ULONG ul = 0; ul < ulArity; ul++)
		{
			if (FMotionHazard(memory_pool, (*pdxln)[ul], peopid, ulOps, pbsPrjCols))
			{
				return true;
			}
		}
	}

	return false;
}

BOOL
CTranslatorExprToDXLUtils::FMotionHazardSafeOp
	(
	CDXLNode *pdxln
	)
{
	BOOL fMotionHazardSafeOp = false;
	Edxlopid edxlop = pdxln->GetOperator()->GetDXLOperator();

	switch (edxlop)
	{
		case EdxlopPhysicalSort:
		case EdxlopPhysicalMotionGather:
		case EdxlopPhysicalMaterialize:
			fMotionHazardSafeOp = true;
			break;

		case EdxlopPhysicalAgg:
		{
			CDXLPhysicalAgg *pdxlnPhysicalAgg = CDXLPhysicalAgg::Cast(pdxln->GetOperator());
			if (pdxlnPhysicalAgg->Edxlaggstr() == EdxlaggstrategyHashed)
				fMotionHazardSafeOp = true;
		}
			break;

		default:
			break;
	}

	return fMotionHazardSafeOp;
}
// EOF
