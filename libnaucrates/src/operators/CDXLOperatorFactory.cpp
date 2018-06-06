//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLOperatorFactory.cpp
//
//	@doc:
//		Implementation of the factory methods for creation of DXL elements.
//---------------------------------------------------------------------------

#include "gpos/string/CWStringConst.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/common/clibwrapper.h"

#include "naucrates/md/CMDIdColStats.h"
#include "naucrates/md/CMDIdRelStats.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDIdGPDBCtas.h"
#include "naucrates/md/CMDIdCast.h"
#include "naucrates/md/CMDIdScCmp.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLDatumGeneric.h"
#include "naucrates/dxl/operators/CDXLDatumStatsLintMappable.h"
#include "naucrates/dxl/operators/CDXLDatumStatsDoubleMappable.h"
#include "naucrates/dxl/operators/CDXLDatumInt2.h"
#include "naucrates/dxl/operators/CDXLDatumInt4.h"
#include "naucrates/dxl/operators/CDXLDatumInt8.h"
#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/operators/CDXLDatumOid.h"

#include <xercesc/util/NumberFormatException.hpp>

using namespace gpos;
using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

#define GPDXL_GPDB_MDID_COMPONENTS 3
#define GPDXL_DEFAULT_USERID 0

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLTblScan
//
//	@doc:
//		Construct a table scan operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLTblScan
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes & // attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	return GPOS_NEW(memory_pool) CDXLPhysicalTableScan(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLSubqScan
//
//	@doc:
//		Construct a subquery scan operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLSubqScan
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse subquery name from attributes
	const XMLCh *xmlszAlias = ExtractAttrValue
								(
								attrs,
								EdxltokenAlias,
								EdxltokenPhysicalSubqueryScan
								);

	CWStringDynamic *pstrAlias = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl,xmlszAlias);
	

	// create a copy of the string in the CMDName constructor
	CMDName *pmdnameAlias = GPOS_NEW(memory_pool) CMDName(memory_pool, pstrAlias);
	
	GPOS_DELETE(pstrAlias);

	return GPOS_NEW(memory_pool) CDXLPhysicalSubqueryScan(memory_pool, pmdnameAlias);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLResult
//
//	@doc:
//		Construct a result operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLResult
	(
	CDXLMemoryManager *memory_manager_dxl
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLPhysicalResult(memory_pool);
}

//		Construct a hashjoin operator
CDXLPhysical*
CDXLOperatorFactory::MakeDXLHashJoin
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	const XMLCh *xmlszJoinType = ExtractAttrValue
									(
									attrs,
									EdxltokenJoinType,
									EdxltokenPhysicalHashJoin
									);
	
	EdxlJoinType join_type = EdxljtParseJoinType(xmlszJoinType, CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalHashJoin));
	
	return GPOS_NEW(memory_pool) CDXLPhysicalHashJoin(memory_pool, join_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLNLJoin
//
//	@doc:
//		Construct a nested loop join operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLNLJoin
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	const XMLCh *xmlszJoinType = ExtractAttrValue
									(
									attrs,
									EdxltokenJoinType,
									EdxltokenPhysicalNLJoin
									);
	
	BOOL fIndexNLJ = false;
	const XMLCh *xmlszIndexNLJ = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenPhysicalNLJoinIndex));
	if (NULL != xmlszIndexNLJ)
	{
		fIndexNLJ = ConvertAttrValueToBool
						(
						memory_manager_dxl,
						xmlszIndexNLJ,
						EdxltokenPhysicalNLJoinIndex,
						EdxltokenPhysicalNLJoin
						);
	}

	EdxlJoinType join_type = EdxljtParseJoinType(xmlszJoinType, CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalNLJoin));
	
	return GPOS_NEW(memory_pool) CDXLPhysicalNLJoin(memory_pool, join_type, fIndexNLJ);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLMergeJoin
//
//	@doc:
//		Construct a merge join operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLMergeJoin
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	const XMLCh *xmlszJoinType = ExtractAttrValue
									(
									attrs,
									EdxltokenJoinType,
									EdxltokenPhysicalMergeJoin
									);
	
	EdxlJoinType join_type = EdxljtParseJoinType(xmlszJoinType, CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalMergeJoin));
	
	BOOL fUniqueOuter = ExtractConvertAttrValueToBool
								(
								memory_manager_dxl,
								attrs,
								EdxltokenMergeJoinUniqueOuter,
								EdxltokenPhysicalMergeJoin
								);
	
	return GPOS_NEW(memory_pool) CDXLPhysicalMergeJoin(memory_pool, join_type, fUniqueOuter);
}

//		Construct a gather motion operator
CDXLPhysical *
CDXLOperatorFactory::MakeDXLGatherMotion
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{	
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	CDXLPhysicalGatherMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalGatherMotion(memory_pool);
	SetSegmentInfo(memory_manager_dxl, dxl_op, attrs, EdxltokenPhysicalGatherMotion);

	return dxl_op;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLBroadcastMotion
//
//	@doc:
//		Construct a broadcast motion operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLBroadcastMotion
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	CDXLPhysicalBroadcastMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalBroadcastMotion(memory_pool);
	SetSegmentInfo(memory_manager_dxl, dxl_op, attrs, EdxltokenPhysicalBroadcastMotion);
	
	return dxl_op;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLRedistributeMotion
//
//	@doc:
//		Construct a redistribute motion operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLRedistributeMotion
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	BOOL fDuplicateSensitive = false;
	
	const XMLCh *xmlszDuplicateSensitive = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDuplicateSensitive));
	if (NULL != xmlszDuplicateSensitive)
	{
		fDuplicateSensitive = ConvertAttrValueToBool(memory_manager_dxl, xmlszDuplicateSensitive, EdxltokenDuplicateSensitive, EdxltokenPhysicalRedistributeMotion);
	}
		
	CDXLPhysicalRedistributeMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalRedistributeMotion(memory_pool, fDuplicateSensitive);
	SetSegmentInfo(memory_manager_dxl, dxl_op, attrs, EdxltokenPhysicalRedistributeMotion);
	
	
	
	return dxl_op;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLRoutedMotion
//
//	@doc:
//		Construct a routed motion operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLRoutedMotion
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	ULONG ulSegmentIdCol = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenSegmentIdCol, EdxltokenPhysicalRoutedDistributeMotion);
	
	CDXLPhysicalRoutedDistributeMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalRoutedDistributeMotion(memory_pool, ulSegmentIdCol);
	SetSegmentInfo(memory_manager_dxl, dxl_op, attrs, EdxltokenPhysicalRoutedDistributeMotion);
	
	return dxl_op;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLRandomMotion
//
//	@doc:
//		Construct a random motion operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLRandomMotion
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	BOOL fDuplicateSensitive = false;

	const XMLCh *xmlszDuplicateSensitive = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDuplicateSensitive));
	if (NULL != xmlszDuplicateSensitive)
	{
		fDuplicateSensitive = ConvertAttrValueToBool(memory_manager_dxl, xmlszDuplicateSensitive, EdxltokenDuplicateSensitive, EdxltokenPhysicalRandomMotion);
	}

	CDXLPhysicalRandomMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalRandomMotion(memory_pool, fDuplicateSensitive);
	SetSegmentInfo(memory_manager_dxl, dxl_op, attrs, EdxltokenPhysicalRandomMotion);
	
	return dxl_op;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLAppend
//	@doc:
//		Construct an Append operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLAppend
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	BOOL fIsTarget = ExtractConvertAttrValueToBool
						(
						memory_manager_dxl,
						attrs,
						EdxltokenAppendIsTarget,
						EdxltokenPhysicalAppend
						);
	
	BOOL fIsZapped = ExtractConvertAttrValueToBool
						(
						memory_manager_dxl,
						attrs,
						EdxltokenAppendIsZapped,
						EdxltokenPhysicalAppend
						);

	return GPOS_NEW(memory_pool) CDXLPhysicalAppend(memory_pool, fIsTarget, fIsZapped);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLLimit
//	@doc:
//		Construct a Limit operator
//
//---------------------------------------------------------------------------
CDXLPhysical*
CDXLOperatorFactory::MakeDXLLimit
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes & // attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLPhysicalLimit(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLLimitCount
//
//	@doc:
//		Construct a Limit Count operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::MakeDXLLimitCount
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes & // attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLScalarLimitCount(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLLimitOffset
//
//	@doc:
//		Construct a Limit Offset operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::MakeDXLLimitOffset
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes & // attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLScalarLimitOffset(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLAgg
//
//	@doc:
//		Construct an aggregate operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLAgg
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes & attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	const XMLCh *xmlszAggStrategy = ExtractAttrValue
										(
										attrs,
										EdxltokenAggStrategy,
										EdxltokenPhysicalAggregate
										);
	
	EdxlAggStrategy agg_strategy_dxl = EdxlaggstrategySentinel;
	
	if (0 == XMLString::compareString
							(
							CDXLTokens::XmlstrToken(EdxltokenAggStrategyPlain),
							xmlszAggStrategy
							))
	{
		agg_strategy_dxl = EdxlaggstrategyPlain;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggStrategySorted),
									xmlszAggStrategy))
	{
		agg_strategy_dxl = EdxlaggstrategySorted;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggStrategyHashed),
									xmlszAggStrategy))
	{
		agg_strategy_dxl = EdxlaggstrategyHashed;
	}
	else
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenAggStrategy)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalAggregate)->GetBuffer()
			);		
	}
	
	BOOL stream_safe = false;

	const XMLCh *xmlszStreamSafe = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenAggStreamSafe));
	if (NULL != xmlszStreamSafe)
	{
		stream_safe = ConvertAttrValueToBool
						(
						memory_manager_dxl,
						xmlszStreamSafe,
						EdxltokenAggStreamSafe,
						EdxltokenPhysicalAggregate
						);
	}

	return GPOS_NEW(memory_pool) CDXLPhysicalAgg(memory_pool, agg_strategy_dxl, stream_safe);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLSort
//
//	@doc:
//		Construct a sort operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLSort
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse discard duplicates and nulls first properties from the attributes
	BOOL fDiscardDuplicates = ExtractConvertAttrValueToBool
								(
								memory_manager_dxl,
								attrs,
								EdxltokenSortDiscardDuplicates,
								EdxltokenPhysicalSort
								);
	
	return GPOS_NEW(memory_pool) CDXLPhysicalSort(memory_pool, fDiscardDuplicates);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLMaterialize
//
//	@doc:
//		Construct a materialize operator
//
//---------------------------------------------------------------------------
CDXLPhysical *
CDXLOperatorFactory::MakeDXLMaterialize
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse spooling info from the attributes
	
	CDXLPhysicalMaterialize *pdxlopMat = NULL;
	
	// is this a multi-slice spool
	BOOL fEagerMaterialize = ExtractConvertAttrValueToBool
								(
								memory_manager_dxl,
								attrs,
								EdxltokenMaterializeEager,
								EdxltokenPhysicalMaterialize
								);
	
	if (1 == attrs.getLength())
	{
		// no spooling info specified -> create a non-spooling materialize operator
		pdxlopMat = GPOS_NEW(memory_pool) CDXLPhysicalMaterialize(memory_pool, fEagerMaterialize);
	}
	else
	{
		// parse spool id
		ULONG ulSpoolId = ExtractConvertAttrValueToUlong
							(
							memory_manager_dxl,
							attrs,
							EdxltokenSpoolId,
							EdxltokenPhysicalMaterialize
							);
	
		// parse id of executor slice
		INT iExecutorSlice = ExtractConvertAttrValueToInt
									(
									memory_manager_dxl,
									attrs,
									EdxltokenExecutorSliceId,
									EdxltokenPhysicalMaterialize
									);
		
		ULONG ulConsumerSlices = ExtractConvertAttrValueToUlong
									(
									memory_manager_dxl,
									attrs,
									EdxltokenConsumerSliceCount,
									EdxltokenPhysicalMaterialize
									);
	
		pdxlopMat = GPOS_NEW(memory_pool) CDXLPhysicalMaterialize(memory_pool, fEagerMaterialize, ulSpoolId, iExecutorSlice, ulConsumerSlices);
	}
	
	return pdxlopMat;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLScalarCmp
//
//	@doc:
//		Construct a scalar comparison operator
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLScalarCmp
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	 // get comparison operator from attributes
	const XMLCh *xmlszCmpOp = ExtractAttrValue(attrs, EdxltokenComparisonOp, EdxltokenScalarComp);
	
	// parse op no and function id
	IMDId *pmdidOpNo = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenOpNo, EdxltokenScalarComp);
	
	// parse comparison operator from string
	CWStringDynamic *pstrCompOpName = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszCmpOp);

	
	// copy dynamic string into const string
	CWStringConst *pstrCompOpNameCopy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, pstrCompOpName->GetBuffer());

	// cleanup
	GPOS_DELETE(pstrCompOpName);
	
	return GPOS_NEW(memory_pool) CDXLScalarComp(memory_pool, pmdidOpNo, pstrCompOpNameCopy);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLDistinctCmp
//
//	@doc:
//		Construct a scalar distinct comparison operator
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLDistinctCmp
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse operator and function id
	IMDId *pmdidOpNo = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenOpNo,EdxltokenScalarDistinctComp);

	return GPOS_NEW(memory_pool) CDXLScalarDistinctComp(memory_pool, pmdidOpNo);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLOpExpr
//
//	@doc:
//		Construct a scalar OpExpr
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLOpExpr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	// get scalar OpExpr from attributes
	const XMLCh *xmlszOpName = ExtractAttrValue
									(
									attrs,
									EdxltokenOpName,
									EdxltokenScalarOpExpr
									);

	IMDId *pmdidOpNo = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenOpNo,
							EdxltokenScalarOpExpr
							);

	IMDId *pmdidReturnType = NULL;
	const XMLCh *xmlszReturnType = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenOpType));

	if (NULL != xmlszReturnType)
	{
		pmdidReturnType = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenOpType, EdxltokenScalarOpExpr);
	}
	
	CWStringDynamic *pstrValue = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszOpName);
	CWStringConst *pstrValueCopy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, pstrValue->GetBuffer());
	GPOS_DELETE(pstrValue);

	return GPOS_NEW(memory_pool) CDXLScalarOpExpr(memory_pool, pmdidOpNo, pmdidReturnType, pstrValueCopy);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLArrayComp
//
//	@doc:
//		Construct a scalar array comparison
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLArrayComp
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	 // get attributes
	const XMLCh *xmlszOpName = ExtractAttrValue
								(
								attrs,
								EdxltokenOpName,
								EdxltokenScalarArrayComp
								);
	
	const XMLCh *xmlszOpType = ExtractAttrValue
									(
									attrs,
									EdxltokenOpType,
									EdxltokenScalarArrayComp
									);

	// parse operator no and function id
	IMDId *pmdidOpNo = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenOpNo,
							EdxltokenScalarArrayComp
							);

	EdxlArrayCompType edxlarraycomptype = Edxlarraycomptypeany;

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpTypeAll), xmlszOpType))
	{
		edxlarraycomptype = Edxlarraycomptypeall;
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpTypeAny), xmlszOpType))
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenOpType)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenScalarArrayComp)->GetBuffer()
			);
	}

	CWStringDynamic *pstrOpName = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszOpName);
	CWStringConst *pstrOpNameCopy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, pstrOpName->GetBuffer());
	GPOS_DELETE(pstrOpName);

	return GPOS_NEW(memory_pool) CDXLScalarArrayComp(memory_pool, pmdidOpNo, pstrOpNameCopy, edxlarraycomptype);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLBoolExpr
//
//	@doc:
//		Construct a scalar BoolExpr
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLBoolExpr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const EdxlBoolExprType edxlboolexprType
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool,	edxlboolexprType);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLBooleanTest
//
//	@doc:
//		Construct a scalar BooleanTest
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLBooleanTest
	(
	CDXLMemoryManager *memory_manager_dxl,
	const EdxlBooleanTestType edxlbooleantesttype
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLScalarBooleanTest(memory_pool,	edxlbooleantesttype);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLSubPlan
//
//	@doc:
//		Construct a SubPlan node
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLSubPlan
	(
	CDXLMemoryManager *memory_manager_dxl,
	IMDId *pmdid,
	DrgPdxlcr *pdrgdxlcr,
	EdxlSubPlanType edxlsubplantype,
	CDXLNode *pdxlnTestExpr
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	return GPOS_NEW(memory_pool) CDXLScalarSubPlan(memory_pool, pmdid, pdrgdxlcr, edxlsubplantype, pdxlnTestExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLNullTest
//
//	@doc:
//		Construct a scalar NullTest
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLNullTest
	(
	CDXLMemoryManager *memory_manager_dxl,
	const BOOL fIsNull
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLScalarNullTest(memory_pool, fIsNull);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLCast
//
//	@doc:
//		Construct a scalar RelabelType
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLCast
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse type id and function id
	IMDId *mdid_type = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenTypeId,
							EdxltokenScalarCast
							);

	IMDId *mdid_func = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenFuncId,
							EdxltokenScalarCast
							);

	return GPOS_NEW(memory_pool) CDXLScalarCast(memory_pool, mdid_type, mdid_func);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLCoerceToDomain
//
//	@doc:
//		Construct a scalar coerce
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLCoerceToDomain
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	// parse type id and function id
	IMDId *mdid_type = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarCoerceToDomain);
	INT type_modifier = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenTypeMod, EdxltokenScalarCoerceToDomain, true, IDefaultTypeModifier);
	ULONG ulCoercionForm = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenCoercionForm, EdxltokenScalarCoerceToDomain);
	INT iLoc = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenLocation, EdxltokenScalarCoerceToDomain);

	return GPOS_NEW(memory_pool) CDXLScalarCoerceToDomain(memory_pool, mdid_type, type_modifier, (EdxlCoercionForm) ulCoercionForm, iLoc);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLCoerceViaIO
//
//	@doc:
//		Construct a scalar coerce
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLCoerceViaIO
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	// parse type id and function id
	IMDId *mdid_type = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarCoerceViaIO);
	INT type_modifier = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenTypeMod, EdxltokenScalarCoerceViaIO, true, IDefaultTypeModifier);
	ULONG ulCoercionForm = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenCoercionForm, EdxltokenScalarCoerceViaIO);
	INT iLoc = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenLocation, EdxltokenScalarCoerceViaIO);

	return GPOS_NEW(memory_pool) CDXLScalarCoerceViaIO(memory_pool, mdid_type, type_modifier, (EdxlCoercionForm) ulCoercionForm, iLoc);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLArrayCoerceExpr
//
//	@doc:
//		Construct a scalar array coerce expression
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLArrayCoerceExpr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	IMDId *pmdidElementFunc = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenElementFunc, EdxltokenScalarArrayCoerceExpr);
	IMDId *mdid_type = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarArrayCoerceExpr);
	INT type_modifier = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenTypeMod, EdxltokenScalarArrayCoerceExpr, true, IDefaultTypeModifier);
	BOOL fIsExplicit = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenIsExplicit, EdxltokenScalarArrayCoerceExpr);
	ULONG ulCoercionForm = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenCoercionForm, EdxltokenScalarArrayCoerceExpr);
	INT iLoc = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenLocation, EdxltokenScalarArrayCoerceExpr);

	return GPOS_NEW(memory_pool) CDXLScalarArrayCoerceExpr(memory_pool, pmdidElementFunc, mdid_type, type_modifier, fIsExplicit, (EdxlCoercionForm) ulCoercionForm, iLoc);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLConstValue
//
//	@doc:
//		Construct a scalar Const
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLConstValue
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	CDXLDatum *datum_dxl = GetDatumVal(memory_manager_dxl, attrs, EdxltokenScalarConstValue);
	
	return GPOS_NEW(memory_pool) CDXLScalarConstValue(memory_pool, datum_dxl);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLIfStmt
//
//	@doc:
//		Construct an if statement operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::MakeDXLIfStmt
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	// get the type id
	IMDId *mdid_type = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenTypeId,
							EdxltokenScalarIfStmt
							);

	return GPOS_NEW(memory_pool) CDXLScalarIfStmt(memory_pool, mdid_type);
}


//		Construct an funcexpr operator
CDXLScalar*
CDXLOperatorFactory::MakeDXLFuncExpr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	IMDId *mdid_func = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenFuncId,
							EdxltokenScalarFuncExpr
							);

	BOOL fRetset = ExtractConvertAttrValueToBool
					(
					memory_manager_dxl,
					attrs,
					EdxltokenFuncRetSet,
					EdxltokenScalarFuncExpr
					);

	IMDId *mdid_return_type = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenTypeId,
							EdxltokenScalarFuncExpr
							);

	INT type_modifier = ExtractConvertAttrValueToInt
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeMod,
						EdxltokenScalarCast,
						true,
						IDefaultTypeModifier
						);

	return GPOS_NEW(memory_pool) CDXLScalarFuncExpr(memory_pool, mdid_func, mdid_return_type, type_modifier, fRetset);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLAggFunc
//
//	@doc:
//		Construct an AggRef operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::MakeDXLAggFunc
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	IMDId *pmdidAgg = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenAggrefOid,
							EdxltokenScalarAggref
							);

	const XMLCh *xmlszStage  = ExtractAttrValue
									(
									attrs,
									EdxltokenAggrefStage,
									EdxltokenScalarAggref
									);
	
	BOOL fDistinct = ExtractConvertAttrValueToBool
						(
						memory_manager_dxl,
						attrs,
						EdxltokenAggrefDistinct,
						EdxltokenScalarAggref
						);

	EdxlAggrefStage edxlaggstage = EdxlaggstageFinal;

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggrefStageNormal), xmlszStage))
	{
		edxlaggstage = EdxlaggstageNormal;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggrefStagePartial), xmlszStage))
	{
		edxlaggstage = EdxlaggstagePartial;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggrefStageIntermediate), xmlszStage))
	{
		edxlaggstage = EdxlaggstageIntermediate;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggrefStageFinal), xmlszStage))
	{
		edxlaggstage = EdxlaggstageFinal;
	}
	else
	{
		// turn Xerces exception in optimizer exception	
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenAggrefStage)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenScalarAggref)->GetBuffer()
			);
	}

	IMDId *pmdidResolvedRetType = NULL;
	const XMLCh *xmlszReturnType = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenTypeId));
	if (NULL != xmlszReturnType)
	{
		pmdidResolvedRetType = PmdidFromAttrs
									(
									memory_manager_dxl,
									attrs,
									EdxltokenTypeId,
									EdxltokenScalarAggref
									);
	}

	return GPOS_NEW(memory_pool) CDXLScalarAggref(memory_pool, pmdidAgg, pmdidResolvedRetType, fDistinct, edxlaggstage);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseDXLFrameBoundary
//
//	@doc:
//		Parse the frame boundary
//
//---------------------------------------------------------------------------
EdxlFrameBoundary
CDXLOperatorFactory::ParseDXLFrameBoundary
	(
	const Attributes& attrs,
	Edxltoken token_type
	)
{
	const XMLCh *xmlszBoundary  = ExtractAttrValue(attrs, token_type, EdxltokenWindowFrame);

	EdxlFrameBoundary edxlfb = EdxlfbSentinel;
	ULONG rgrgulMapping[][2] =
					{
					{EdxlfbUnboundedPreceding, EdxltokenWindowBoundaryUnboundedPreceding},
					{EdxlfbBoundedPreceding, EdxltokenWindowBoundaryBoundedPreceding},
					{EdxlfbCurrentRow, EdxltokenWindowBoundaryCurrentRow},
					{EdxlfbUnboundedFollowing, EdxltokenWindowBoundaryUnboundedFollowing},
					{EdxlfbBoundedFollowing, EdxltokenWindowBoundaryBoundedFollowing},
					{EdxlfbDelayedBoundedPreceding, EdxltokenWindowBoundaryDelayedBoundedPreceding},
					{EdxlfbDelayedBoundedFollowing, EdxltokenWindowBoundaryDelayedBoundedFollowing}
					};

	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		Edxltoken edxltk = (Edxltoken) pulElem[1];
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(edxltk), xmlszBoundary))
		{
			edxlfb = (EdxlFrameBoundary) pulElem[0];
			break;
		}
	}

	if (EdxlfbSentinel == edxlfb)
	{
		// turn Xerces exception in optimizer exception
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(token_type)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenWindowFrame)->GetBuffer()
			);
	}

	return edxlfb;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseDXLFrameSpec
//
//	@doc:
//		Parse the frame specification
//
//---------------------------------------------------------------------------
EdxlFrameSpec
CDXLOperatorFactory::ParseDXLFrameSpec
	(
	const Attributes& attrs
	)
{
	const XMLCh *xmlszfs  = ExtractAttrValue(attrs, EdxltokenWindowFrameSpec, EdxltokenWindowFrame);

	EdxlFrameSpec edxlfb = EdxlfsSentinel;
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFSRow), xmlszfs))
	{
		edxlfb = EdxlfsRow;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFSRange), xmlszfs))
	{
		edxlfb = EdxlfsRange;
	}
	else
	{
		// turn Xerces exception in optimizer exception
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenWindowFrameSpec)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenWindowFrame)->GetBuffer()
			);
	}

	return edxlfb;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseFrameExclusionStrategy
//
//	@doc:
//		Parse the frame exclusion strategy
//
//---------------------------------------------------------------------------
EdxlFrameExclusionStrategy
CDXLOperatorFactory::ParseFrameExclusionStrategy
	(
	const Attributes& attrs
	)
{
	const XMLCh *xmlszfes  = ExtractAttrValue(attrs, EdxltokenWindowExclusionStrategy, EdxltokenWindowFrame);

	ULONG rgrgulMapping[][2] =
			{
			{EdxlfesNone, EdxltokenWindowESNone},
			{EdxlfesNulls, EdxltokenWindowESNulls},
			{EdxlfesCurrentRow, EdxltokenWindowESCurrentRow},
			{EdxlfesGroup, EdxltokenWindowESGroup},
			{EdxlfesTies, EdxltokenWindowESTies}
			};

	EdxlFrameExclusionStrategy edxlfes = EdxlfesSentinel;
	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		Edxltoken edxltk = (Edxltoken) pulElem[1];
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(edxltk), xmlszfes))
		{
			edxlfes = (EdxlFrameExclusionStrategy) pulElem[0];
			break;
		}
	}

	if (EdxlfesSentinel == edxlfes)
	{
		// turn Xerces exception in optimizer exception
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenWindowExclusionStrategy)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenWindowFrame)->GetBuffer()
			);
	}

	return edxlfes;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLArray
//
//	@doc:
//		Construct an array operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::MakeDXLArray
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	IMDId *pmdidElem = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenArrayElementType, EdxltokenScalarArray);
	IMDId *pmdidArray = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenArrayType, EdxltokenScalarArray);
	BOOL fMultiDimensional = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenArrayMultiDim, EdxltokenScalarArray);

	return GPOS_NEW(memory_pool) CDXLScalarArray(memory_pool, pmdidElem, pmdidArray, fMultiDimensional);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLScalarIdent
//
//	@doc:
//		Construct a scalar identifier operator
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLScalarIdent
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	CDXLColRef *pdxlcr = MakeDXLColRef(memory_manager_dxl, attrs, EdxltokenScalarIdent);

	return GPOS_NEW(memory_pool) CDXLScalarIdent(memory_pool, pdxlcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLProjElem
//
//	@doc:
//		Construct a proj elem operator
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLProjElem
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse alias from attributes	
	const XMLCh *xmlszAlias = ExtractAttrValue
								(
								attrs,
								EdxltokenAlias,
								EdxltokenScalarProjElem
								);
	
	// parse column id
	ULONG ulId = ExtractConvertAttrValueToUlong
					(
					memory_manager_dxl,
					attrs,
					EdxltokenColId,
					EdxltokenScalarProjElem
					);
	
	CWStringDynamic *pstrAlias = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszAlias);

	// create a copy of the string in the CMDName constructor
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pstrAlias);
	
	GPOS_DELETE(pstrAlias);
	
	return GPOS_NEW(memory_pool) CDXLScalarProjElem(memory_pool, ulId, mdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLHashExpr
//
//	@doc:
//		Construct a hash expr operator
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLHashExpr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// get column type id and type name from attributes

	IMDId *mdid_type = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenTypeId,
							EdxltokenScalarHashExpr
							);

	return GPOS_NEW(memory_pool) CDXLScalarHashExpr(memory_pool, mdid_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLSortCol
//
//	@doc:
//		Construct a sorting column description
//
//---------------------------------------------------------------------------
CDXLScalar *
CDXLOperatorFactory::MakeDXLSortCol
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// get column id from attributes
	ULONG col_id = ExtractConvertAttrValueToUlong
						(
						memory_manager_dxl,
						attrs,
						EdxltokenColId,
						EdxltokenScalarSortCol
						);

	// get sorting operator name
	const XMLCh *xmlszSortOpName = ExtractAttrValue
										(
										attrs,
										EdxltokenSortOpName,
										EdxltokenScalarSortCol
										);
	CWStringDynamic *pstrSortOpName = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszSortOpName);

	// get null first property
	BOOL fNullsFirst = ExtractConvertAttrValueToBool
						(
						memory_manager_dxl,
						attrs,
						EdxltokenSortNullsFirst,
						EdxltokenPhysicalSort
						);
	
	// parse sorting operator id
	IMDId *pmdidSortOp = PmdidFromAttrs
								(
								memory_manager_dxl,
								attrs,
								EdxltokenSortOpId,
								EdxltokenPhysicalSort
								);

	// copy dynamic string into const string
	CWStringConst *pstrSortOpNameCopy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, pstrSortOpName->GetBuffer());

	GPOS_DELETE(pstrSortOpName);

	return GPOS_NEW(memory_pool) CDXLScalarSortCol(memory_pool, col_id, pmdidSortOp, pstrSortOpNameCopy, fNullsFirst);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLOperatorCost
//
//	@doc:
//		Construct a cost estimates element
//
//---------------------------------------------------------------------------
CDXLOperatorCost *
CDXLOperatorFactory::MakeDXLOperatorCost
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	const XMLCh *xmlszStartupCost = ExtractAttrValue
										(
										attrs,
										EdxltokenStartupCost,
										EdxltokenCost
										);
	
	const XMLCh *xmlszTotalCost = ExtractAttrValue
										(
										attrs,
										EdxltokenTotalCost,
										EdxltokenCost
										);
	
	const XMLCh *xmlszRows = ExtractAttrValue
								(
								attrs,
								EdxltokenRows,
								EdxltokenCost
								);
	
	const XMLCh *xmlszWidth = ExtractAttrValue
								(
								attrs,
								EdxltokenWidth,
								EdxltokenCost
								);
	
	CWStringDynamic *startup_cost_str = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszStartupCost);
	CWStringDynamic *total_cost_str = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszTotalCost);
	CWStringDynamic *rows_out_str = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszRows);
	CWStringDynamic *width_str = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszWidth);
	
	return GPOS_NEW(memory_pool) CDXLOperatorCost
						(
						startup_cost_str,
						total_cost_str,
						rows_out_str,
						width_str
						);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLTableDescr
//
//	@doc:
//		Construct a table descriptor
//
//---------------------------------------------------------------------------
CDXLTableDescr *
CDXLOperatorFactory::MakeDXLTableDescr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse table descriptor from attributes
	const XMLCh *xmlszTableName = ExtractAttrValue
									(
									attrs,
									EdxltokenTableName,
									EdxltokenTableDescr
									);

	CMDName *mdname = CDXLUtils::CreateMDNameFromXMLChar(memory_manager_dxl, xmlszTableName);
	
	// parse metadata id
	IMDId *pmdid = PmdidFromAttrs
						(
						memory_manager_dxl,
						attrs,
						EdxltokenMdid,
						EdxltokenTableDescr
						);

	// parse execute as user value if the attribute is specified
	ULONG ulUserId = GPDXL_DEFAULT_USERID;
	const XMLCh *xmlszExecuteAsUser = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenExecuteAsUser));

	if (NULL != xmlszExecuteAsUser)
	{
		ulUserId = ConvertAttrValueToUlong(memory_manager_dxl, xmlszExecuteAsUser, EdxltokenExecuteAsUser, EdxltokenTableDescr);
	}
					
	return GPOS_NEW(memory_pool) CDXLTableDescr(memory_pool, pmdid, mdname, ulUserId);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLIndexDescr
//
//	@doc:
//		Construct an index descriptor
//
//---------------------------------------------------------------------------
CDXLIndexDescr *
CDXLOperatorFactory::MakeDXLIndexDescr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	// parse index descriptor from attributes
	const XMLCh *xmlszIndexName = ExtractAttrValue
									(
									attrs,
									EdxltokenIndexName,
									EdxltokenIndexDescr
									);

	CWStringDynamic *pstrIndexName = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszIndexName);

	// parse metadata id
	IMDId *pmdid = PmdidFromAttrs
						(
						memory_manager_dxl,
						attrs,
						EdxltokenMdid,
						EdxltokenIndexDescr
						);

	// create a copy of the string in the CMDName constructor
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pstrIndexName);
	GPOS_DELETE(pstrIndexName);

	return GPOS_NEW(memory_pool) CDXLIndexDescr(memory_pool, pmdid, mdname);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLColumnDescr
//
//	@doc:
//		Construct a column descriptor
//
//---------------------------------------------------------------------------
CDXLColDescr *
CDXLOperatorFactory::MakeDXLColumnDescr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	// parse column name from attributes
	const XMLCh *xmlszColumnName = ExtractAttrValue
										(
										attrs,
										EdxltokenColName,
										EdxltokenColDescr
										);

	// parse column id
	ULONG ulId = ExtractConvertAttrValueToUlong
					(
					memory_manager_dxl,
					attrs,
					EdxltokenColId,
					EdxltokenColDescr
					);
	
	// parse attno
	INT iAttno = ExtractConvertAttrValueToInt
					(
					memory_manager_dxl,
					attrs,
					EdxltokenAttno,
					EdxltokenColDescr
					);
	
	if (0 == iAttno)
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenAttno)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenColDescr)->GetBuffer()
			);
	}
	
	// parse column type id
	IMDId *mdid_type = PmdidFromAttrs
							(
							memory_manager_dxl,
							attrs,
							EdxltokenTypeId,
							EdxltokenColDescr
							);

	// parse optional type modifier from attributes
	INT type_modifier = ExtractConvertAttrValueToInt
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeMod,
						EdxltokenColDescr,
						true,
						IDefaultTypeModifier
						);

	BOOL fColDropped = false;
	
	const XMLCh *xmlszColDropped = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColDropped));
	
	if (NULL != xmlszColDropped)
	{
		// attribute is present: get value
		fColDropped = ConvertAttrValueToBool
						(
						memory_manager_dxl,
						xmlszColDropped,
						EdxltokenColDropped,
						EdxltokenColDescr
						);
	}
	
	ULONG ulColLen = ULONG_MAX;

	// parse column length from attributes
	const XMLCh *xmlszColumnLength =  attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColWidth));

	if (NULL != xmlszColumnLength)
	{
		ulColLen = ConvertAttrValueToUlong
					(
					memory_manager_dxl,
					xmlszColumnLength,
					EdxltokenColWidth,
					EdxltokenColDescr
					);
	}

	CWStringDynamic *pstrColumnName = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl,xmlszColumnName);

	// create a copy of the string in the CMDName constructor
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pstrColumnName);
	
	GPOS_DELETE(pstrColumnName);
	
	return GPOS_NEW(memory_pool) CDXLColDescr(memory_pool, mdname, ulId, iAttno, mdid_type, type_modifier, fColDropped, ulColLen);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeDXLColRef
//
//	@doc:
//		Construct a column reference
//
//---------------------------------------------------------------------------
CDXLColRef *
CDXLOperatorFactory::MakeDXLColRef
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	// parse column name from attributes
	const XMLCh *xmlszColumnName = ExtractAttrValue
									(
									attrs,
									EdxltokenColName,
									edxltokenElement
									);

	// parse column id
	ULONG ulId = 0;
	const XMLCh *xmlszColumnId = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColId));
	if(NULL == xmlszColumnId)
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLMissingAttribute,
			CDXLTokens::GetDXLTokenStr(EdxltokenColRef)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
			);
	}
	
	ulId = XMLString::parseInt(xmlszColumnId, memory_manager_dxl);
		
	CWStringDynamic *pstrColumnName =  CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl,xmlszColumnName);

	// create a copy of the string in the CMDName constructor
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pstrColumnName);
	
	GPOS_DELETE(pstrColumnName);

	IMDId *mdid_type = PmdidFromAttrs
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeId,
						edxltokenElement
						);

	// parse optional type modifier
	INT type_modifier = ExtractConvertAttrValueToInt
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeMod,
						edxltokenElement,
						true,
						IDefaultTypeModifier
						);
	
	return GPOS_NEW(memory_pool) CDXLColRef(memory_pool, mdname, ulId, mdid_type, type_modifier);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseOutputSegId
//
//	@doc:
//		Parse an output segment index
//
//---------------------------------------------------------------------------
INT
CDXLOperatorFactory::ParseOutputSegId
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get output segment index from attributes
	const XMLCh *xmlszSegId = ExtractAttrValue
									(
									attrs,
									EdxltokenSegId,
									EdxltokenSegment
									);

	// parse segment id from string
	INT iSegId = -1;
	try
	{
		iSegId = XMLString::parseInt(xmlszSegId, memory_manager_dxl);
	}
	catch (const NumberFormatException& toCatch)
	{
		// turn Xerces exception into GPOS exception
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenSegId)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenSegment)->GetBuffer()
			);
	}

	return iSegId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractAttrValue
//
//	@doc:
//    	Extracts the value for the given attribute.
// 		If there is no such attribute defined, and the given optional
// 		flag is set to false then it will raise an exception
//---------------------------------------------------------------------------
const XMLCh *
CDXLOperatorFactory::ExtractAttrValue
	(
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional
	)
{
	const XMLCh *xmlszAttributeVal = attrs.getValue(CDXLTokens::XmlstrToken(edxltokenAttr));

	if (NULL == xmlszAttributeVal && !fOptional)
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLMissingAttribute,
			CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
			);
	}

	return xmlszAttributeVal;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToUlong
//
//	@doc:
//	  	Converts the attribute value to ULONG
//
//---------------------------------------------------------------------------
ULONG
CDXLOperatorFactory::ConvertAttrValueToUlong
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);
	ULONG ulId = 0;
	try
	{
		ulId = XMLString::parseInt(xmlszAttributeVal, memory_manager_dxl);
	}
	catch (const NumberFormatException& toCatch)
	{
		// turn Xerces exception into GPOS exception
		GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
		);
	}
	return ulId;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToUllong
//
//	@doc:
//	  	Converts the attribute value to ULLONG
//
//---------------------------------------------------------------------------
ULLONG
CDXLOperatorFactory::ConvertAttrValueToUllong
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);

	CHAR *sz = XMLString::transcode(xmlszAttributeVal, memory_manager_dxl);
	GPOS_ASSERT(NULL != sz);

	CHAR **ppszEnd = NULL;
	LINT liVal = clib::StrToLL(sz, ppszEnd, 10 /*ulBase*/);

	if ((NULL != ppszEnd && sz == *ppszEnd) ||
		LINT_MAX == liVal || LINT_MIN == liVal || 0 > liVal)
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
			);
	}

	XMLString::release(&sz, memory_manager_dxl);

	return (ULLONG) liVal;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToBool
//
//	@doc:
//	  	Converts the attribute value to BOOL
//
//---------------------------------------------------------------------------
BOOL
CDXLOperatorFactory::ConvertAttrValueToBool
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);
	BOOL flag = false;
	CHAR *sz = XMLString::transcode(xmlszAttributeVal, memory_manager_dxl);

	if (0 == strncasecmp(sz, "true", 4))
	{
		flag = true;
	}else if (0 != strncasecmp(sz, "false", 5))
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
			);
	}

	XMLString::release(&sz, memory_manager_dxl);
	return flag;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToInt
//
//	@doc:
//	  	Converts the attribute value from xml string to INT
//
//---------------------------------------------------------------------------
INT
CDXLOperatorFactory::ConvertAttrValueToInt
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);
	INT iId = 0;
	try
	{
		iId = XMLString::parseInt(xmlszAttributeVal, memory_manager_dxl);
	}
	catch (const NumberFormatException& toCatch)
	{
		// turn Xerces exception into GPOS exception
		GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
		);
	}
	return iId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToInt
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into INT
//
//---------------------------------------------------------------------------
INT
CDXLOperatorFactory::ExtractConvertAttrValueToInt
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	INT iDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return iDefaultValue;
	}

	return ConvertAttrValueToInt
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToShortInt
//
//	@doc:
//	  	Converts the attribute value from xml string to short int
//
//---------------------------------------------------------------------------
SINT
CDXLOperatorFactory::ConvertAttrValueToShortInt
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);
	SINT sVal = 0;
	try
	{
		sVal = (SINT) XMLString::parseInt(xmlszAttributeVal, memory_manager_dxl);
	}
	catch (const NumberFormatException& toCatch)
	{
		// turn Xerces exception into GPOS exception
		GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
		);
	}
	return sVal;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToShortInt
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into short
//		int
//
//---------------------------------------------------------------------------
SINT
CDXLOperatorFactory::ExtractConvertAttrValueToShortInt
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	SINT sDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return sDefaultValue;
	}

	return ConvertAttrValueToShortInt
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

// Converts the attribute value from xml string to char
CHAR
CDXLOperatorFactory::ConvertAttrValueToChar
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszVal,
	Edxltoken , // edxltokenAttr,
	Edxltoken  // edxltokenElement
	)
{
	GPOS_ASSERT(xmlszVal != NULL);
	CHAR *sz = XMLString::transcode(xmlszVal, memory_manager_dxl);
	CHAR val = *sz;
	XMLString::release(&sz, memory_manager_dxl);
	return val;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToOid
//
//	@doc:
//	  	Converts the attribute value to OID
//
//---------------------------------------------------------------------------
OID
CDXLOperatorFactory::ConvertAttrValueToOid
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);
	OID oid = 0;
	try
	{
		oid = XMLString::parseInt(xmlszAttributeVal, memory_manager_dxl);
	}
	catch (const NumberFormatException& toCatch)
	{
		// turn Xerces exception into GPOS exception
		GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(edxltokenAttr)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(edxltokenElement)->GetBuffer()
		);
	}
	return oid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToOid
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into OID
//
//---------------------------------------------------------------------------
OID
CDXLOperatorFactory::ExtractConvertAttrValueToOid
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	OID OidDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return OidDefaultValue;
	}

	return ConvertAttrValueToOid(memory_manager_dxl, xmlszValue, edxltokenAttr, edxltokenElement);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToSz
//
//	@doc:
//	  	Converts the string attribute value
//
//---------------------------------------------------------------------------
CHAR *
CDXLOperatorFactory::ConvertAttrValueToSz
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlsz,
	Edxltoken , // edxltokenAttr,
	Edxltoken  // edxltokenElement
	)
{
	GPOS_ASSERT(NULL != xmlsz);
	return XMLString::transcode(xmlsz, memory_manager_dxl);	
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToSz
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into CHAR*
//
//---------------------------------------------------------------------------
CHAR *
CDXLOperatorFactory::ExtractConvertAttrValueToSz
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	CHAR *szDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return szDefaultValue;
	}

	return CDXLOperatorFactory::ConvertAttrValueToSz
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToStr
//
//	@doc:
//	  	Extracts the string value for the given attribute
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLOperatorFactory::ExtractConvertAttrValueToStr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement
											);
	return CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszValue);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToBool
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into BOOL
//
//---------------------------------------------------------------------------
BOOL
CDXLOperatorFactory::ExtractConvertAttrValueToBool
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	BOOL fDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return fDefaultValue;
	}

	return ConvertAttrValueToBool
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToUlong
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into ULONG
//
//---------------------------------------------------------------------------
ULONG
CDXLOperatorFactory::ExtractConvertAttrValueToUlong
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	ULONG ulDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return ulDefaultValue;
	}

	return ConvertAttrValueToUlong
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ExtractConvertAttrValueToUllong
//
//	@doc:
//	  	Extracts the value for the given attribute and converts it into ULLONG
//
//---------------------------------------------------------------------------
ULLONG
CDXLOperatorFactory::ExtractConvertAttrValueToUllong
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	ULLONG ullDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement,
											fOptional
											);

	if (NULL == xmlszValue)
	{
		return ullDefaultValue;
	}

	return ConvertAttrValueToUllong
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseGroupingColId
//
//	@doc:
//		Parse a grouping column id
//
//---------------------------------------------------------------------------
ULONG
CDXLOperatorFactory::ParseGroupingColId
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	const CWStringConst *pstrTokenGroupingCol = CDXLTokens::GetDXLTokenStr(EdxltokenGroupingCol);
	const CWStringConst *pstrTokenColId = CDXLTokens::GetDXLTokenStr(EdxltokenColId);

	// get grouping column id from attributes	
	INT iColId = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenColId, EdxltokenGroupingCol);
	
	if (iColId < 0)
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			pstrTokenColId->GetBuffer(),
			pstrTokenGroupingCol->GetBuffer()
			);
	}
	
	return (ULONG) iColId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidFromAttrs
//
//	@doc:
//		Parse a metadata id object from the XML attributes of the specified element.
//
//---------------------------------------------------------------------------
IMDId *
CDXLOperatorFactory::PmdidFromAttrs
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	IMDId *pmdidDefault
	)
{
	// extract mdid
	const XMLCh *xmlszMdid = ExtractAttrValue(attrs, edxltokenAttr, edxltokenElement, fOptional);

	if (NULL == xmlszMdid)
	{
		if (NULL != pmdidDefault)
		{
			pmdidDefault->AddRef();
		}

		return pmdidDefault;
	}
	
	return PmdidFromXMLCh(memory_manager_dxl, xmlszMdid, edxltokenAttr, edxltokenElement);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidFromXMLCh
//
//	@doc:
//		Parse a metadata id object from the XML attributes of the specified element.
//
//---------------------------------------------------------------------------
IMDId *
CDXLOperatorFactory::PmdidFromXMLCh
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszMdid,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	// extract mdid's components: MdidType.Oid.Major.Minor
	XMLStringTokenizer xmlsztok(xmlszMdid, CDXLTokens::XmlstrToken(EdxltokenDotSemicolon));

	GPOS_ASSERT(1 < xmlsztok.countTokens());
	
	// get mdid type from first component
	XMLCh *xmlszMdidType = xmlsztok.nextToken();
	
	// collect the remaining tokens in an array
	DrgPxmlsz *pdrgpxmlsz = GPOS_NEW(memory_manager_dxl->Pmp()) DrgPxmlsz(memory_manager_dxl->Pmp());
	
	XMLCh *xmlsz = NULL;
	while (NULL != (xmlsz = xmlsztok.nextToken()))
	{
		pdrgpxmlsz->Append(xmlsz);
	}
	
	IMDId::EMDIdType emdt = (IMDId::EMDIdType) ConvertAttrValueToUlong(memory_manager_dxl, xmlszMdidType, edxltokenAttr, edxltokenElement);

	IMDId *pmdid = NULL;
	switch (emdt)
	{
		case IMDId::EmdidGPDB:
			pmdid = PmdidGPDB(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidGPDBCtas:
			pmdid = PmdidGPDBCTAS(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidColStats:
			pmdid = PmdidColStats(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidRelStats:
			pmdid = PmdidRelStats(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidCastFunc:
			pmdid = PmdidCastFunc(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidScCmp:
			pmdid = PmdidScCmp(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		default:
			GPOS_ASSERT(!"Unrecognized mdid type");		
	}
	
	pdrgpxmlsz->Release();
	
	return pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidGPDB
//
//	@doc:
//		Construct a GPDB mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdGPDB *
CDXLOperatorFactory::PmdidGPDB
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(GPDXL_GPDB_MDID_COMPONENTS <= pdrgpxmlsz->Size());

	XMLCh *xmlszOid = (*pdrgpxmlsz)[0];
	ULONG oid_colid = ConvertAttrValueToUlong(memory_manager_dxl, xmlszOid, edxltokenAttr, edxltokenElement);

	XMLCh *xmlszVersionMajor = (*pdrgpxmlsz)[1];
	ULONG ulVersionMajor = ConvertAttrValueToUlong(memory_manager_dxl, xmlszVersionMajor, edxltokenAttr, edxltokenElement);

	XMLCh *xmlszVersionMinor = (*pdrgpxmlsz)[2];;
	ULONG ulVersionMinor = ConvertAttrValueToUlong(memory_manager_dxl, xmlszVersionMinor, edxltokenAttr, edxltokenElement);

	// construct metadata id object
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdGPDB(oid_colid, ulVersionMajor, ulVersionMinor);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidGPDBCTAS
//
//	@doc:
//		Construct a GPDB CTAS mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdGPDB *
CDXLOperatorFactory::PmdidGPDBCTAS
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(GPDXL_GPDB_MDID_COMPONENTS <= pdrgpxmlsz->Size());

	XMLCh *xmlszOid = (*pdrgpxmlsz)[0];
	ULONG oid_colid = ConvertAttrValueToUlong(memory_manager_dxl, xmlszOid, edxltokenAttr, edxltokenElement);

	// construct metadata id object
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdGPDBCtas(oid_colid);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidColStats
//
//	@doc:
//		Construct a column stats mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdColStats *
CDXLOperatorFactory::PmdidColStats
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(GPDXL_GPDB_MDID_COMPONENTS + 1 == pdrgpxmlsz->Size());

	CMDIdGPDB *pmdidRel = PmdidGPDB(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	
	XMLCh *xmlszAttno = (*pdrgpxmlsz)[3];
	ULONG ulAttno = ConvertAttrValueToUlong(memory_manager_dxl, xmlszAttno, edxltokenAttr, edxltokenElement);

	// construct metadata id object
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdColStats(pmdidRel, ulAttno);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidRelStats
//
//	@doc:
//		Construct a relation stats mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdRelStats *
CDXLOperatorFactory::PmdidRelStats
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(GPDXL_GPDB_MDID_COMPONENTS == pdrgpxmlsz->Size());

	CMDIdGPDB *pmdidRel = PmdidGPDB(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	
	// construct metadata id object
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdRelStats(pmdidRel);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidCastFunc
//
//	@doc:
//		Construct a cast function mdid from the array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdCast *
CDXLOperatorFactory::PmdidCastFunc
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(2 * GPDXL_GPDB_MDID_COMPONENTS == pdrgpxmlsz->Size());

	CMDIdGPDB *pmdidSrc = PmdidGPDB(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	DrgPxmlsz *pdrgpxmlszDest = GPOS_NEW(memory_manager_dxl->Pmp()) DrgPxmlsz(memory_manager_dxl->Pmp());
	
	for (ULONG ul = GPDXL_GPDB_MDID_COMPONENTS; ul < GPDXL_GPDB_MDID_COMPONENTS * 2; ul++)
	{
		pdrgpxmlszDest->Append((*pdrgpxmlsz)[ul]);
	}
	
	CMDIdGPDB *pmdidDest = PmdidGPDB(memory_manager_dxl, pdrgpxmlszDest, edxltokenAttr, edxltokenElement);
	pdrgpxmlszDest->Release();
	
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdCast(pmdidSrc, pmdidDest);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PmdidScCmp
//
//	@doc:
//		Construct a scalar comparison operator mdid from the array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdScCmp *
CDXLOperatorFactory::PmdidScCmp
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(2 * GPDXL_GPDB_MDID_COMPONENTS + 1 == pdrgpxmlsz->Size());

	CMDIdGPDB *pmdidLeft = PmdidGPDB(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	DrgPxmlsz *pdrgpxmlszRight = GPOS_NEW(memory_manager_dxl->Pmp()) DrgPxmlsz(memory_manager_dxl->Pmp());
	
	for (ULONG ul = GPDXL_GPDB_MDID_COMPONENTS; ul < GPDXL_GPDB_MDID_COMPONENTS * 2 + 1; ul++)
	{
		pdrgpxmlszRight->Append((*pdrgpxmlsz)[ul]);
	}
	
	CMDIdGPDB *pmdidRight = PmdidGPDB(memory_manager_dxl, pdrgpxmlszRight, edxltokenAttr, edxltokenElement);
	
	// parse the comparison type from the last component of the mdid
	XMLCh *xmlszCmpType = (*pdrgpxmlszRight)[pdrgpxmlszRight->Size() - 1];
	IMDType::ECmpType ecmpt = (IMDType::ECmpType) ConvertAttrValueToUlong(memory_manager_dxl, xmlszCmpType, edxltokenAttr, edxltokenElement);
	GPOS_ASSERT(IMDType::EcmptOther > ecmpt);
	
	pdrgpxmlszRight->Release();
	
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdScCmp(pmdidLeft, pmdidRight, ecmpt);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetDatumVal
//
//	@doc:
//		Parses a DXL datum from the given attributes
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumVal
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement
	)
{	
	// get the type id and value of the datum from attributes
	IMDId *pmdid = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenTypeId, edxltokenElement);
	GPOS_ASSERT(IMDId::EmdidGPDB == pmdid->Emdidt());
	CMDIdGPDB *pmdidgpdbd = CMDIdGPDB::PmdidConvert(pmdid);

	// get the type id from string
	BOOL fConstNull = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenIsNull, edxltokenElement);
	BOOL fConstByVal = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenIsByValue, edxltokenElement);

	
	SDXLDatumFactoryElem rgTranslators[] =
	{
		// native support
		{CMDIdGPDB::m_mdidInt2.OidObjectId() , &CDXLOperatorFactory::PdxldatumInt2},
		{CMDIdGPDB::m_mdidInt4.OidObjectId() , &CDXLOperatorFactory::PdxldatumInt4},
		{CMDIdGPDB::m_mdidInt8.OidObjectId() , &CDXLOperatorFactory::PdxldatumInt8},
		{CMDIdGPDB::m_mdidBool.OidObjectId() , &CDXLOperatorFactory::PdxldatumBool},
		{CMDIdGPDB::m_mdidOid.OidObjectId() , &CDXLOperatorFactory::PdxldatumOid},
		// types with long int mapping
		{CMDIdGPDB::m_mdidBPChar.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsLintMappable},
		{CMDIdGPDB::m_mdidVarChar.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsLintMappable},
		{CMDIdGPDB::m_mdidText.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsLintMappable},
		{CMDIdGPDB::m_mdidCash.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsLintMappable},
		// non-integer numeric types
		{CMDIdGPDB::m_mdidNumeric.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidFloat4.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidFloat8.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		// network-related types
		{CMDIdGPDB::m_mdidInet.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidCidr.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidMacaddr.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		// time-related types
		{CMDIdGPDB::m_mdidDate.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTime.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimeTz.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimestamp.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimestampTz.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidAbsTime.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidRelTime.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidInterval.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimeInterval.OidObjectId(), &CDXLOperatorFactory::PdxldatumStatsDoubleMappable}
	};
	
	const ULONG ulTranslators = GPOS_ARRAY_SIZE(rgTranslators);
	// find translator for the datum type
	PfPdxldatum *pf = NULL;
	for (ULONG ul = 0; ul < ulTranslators; ul++)
	{
		SDXLDatumFactoryElem elem = rgTranslators[ul];
		if (pmdidgpdbd->OidObjectId() == elem.oid)
		{
			pf = elem.pf;
			break;
		}
	}
		
	if (NULL == pf)
	{
		// generate a datum of generic type
		return PdxldatumGeneric(memory_manager_dxl, attrs, edxltokenElement, pmdid, fConstNull, fConstByVal);
	}
	else
	{
		return (*pf)(memory_manager_dxl, attrs, edxltokenElement, pmdid, fConstNull, fConstByVal);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumOid
//
//	@doc:
//		Parses a DXL datum of oid type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumOid
(
 CDXLMemoryManager *memory_manager_dxl,
 const Attributes &attrs,
 Edxltoken edxltokenElement,
 IMDId *pmdid,
 BOOL fConstNull ,
 BOOL
#ifdef GPOS_DEBUG
 fConstByVal
#endif // GPOS_DEBUG
 )
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	GPOS_ASSERT(fConstByVal);
	OID oVal = 0;
	if (!fConstNull)
	{
		oVal = ExtractConvertAttrValueToOid(memory_manager_dxl, attrs, EdxltokenValue, edxltokenElement);
	}

	return GPOS_NEW(memory_pool) CDXLDatumOid(memory_pool, pmdid, fConstNull, oVal);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumInt2
//
//	@doc:
//		Parses a DXL datum of int2 type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumInt2
(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement,
	IMDId *pmdid,
	BOOL fConstNull ,
	BOOL
	#ifdef GPOS_DEBUG
	fConstByVal
	#endif // GPOS_DEBUG
 )
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	GPOS_ASSERT(fConstByVal);
	SINT sVal = 0;
	if (!fConstNull)
	{
		sVal = ExtractConvertAttrValueToShortInt(memory_manager_dxl, attrs, EdxltokenValue, edxltokenElement);
	}

	return GPOS_NEW(memory_pool) CDXLDatumInt2(memory_pool, pmdid, fConstNull, sVal);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumInt4
//
//	@doc:
//		Parses a DXL datum of int4 type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumInt4
(
 CDXLMemoryManager *memory_manager_dxl,
 const Attributes &attrs,
 Edxltoken edxltokenElement,
 IMDId *pmdid,
 BOOL fConstNull ,
 BOOL
#ifdef GPOS_DEBUG
 fConstByVal
#endif // GPOS_DEBUG
 )
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	GPOS_ASSERT(fConstByVal);
	INT iVal = 0;
	if (!fConstNull)
	{
		iVal = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenValue, edxltokenElement);
	}
	
	return GPOS_NEW(memory_pool) CDXLDatumInt4(memory_pool, pmdid, fConstNull, iVal);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumInt8
//
//	@doc:
//		Parses a DXL datum of int8 type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumInt8
(
 CDXLMemoryManager *memory_manager_dxl,
 const Attributes &attrs,
 Edxltoken edxltokenElement,
 IMDId *pmdid,
 BOOL fConstNull ,
 BOOL
#ifdef GPOS_DEBUG
 fConstByVal
#endif // GPOS_DEBUG
 )
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	GPOS_ASSERT(fConstByVal);
	LINT lVal = 0;
	if (!fConstNull)
	{
		lVal = LValueFromAttrs(memory_manager_dxl, attrs, EdxltokenValue, edxltokenElement);
	}
	
	return GPOS_NEW(memory_pool) CDXLDatumInt8(memory_pool, pmdid, fConstNull, lVal);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumBool
//
//	@doc:
//		Parses a DXL datum of boolean type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumBool
(
 CDXLMemoryManager *memory_manager_dxl,
 const Attributes &attrs,
 Edxltoken edxltokenElement,
 IMDId *pmdid,
 BOOL fConstNull ,
 BOOL
#ifdef GPOS_DEBUG
 fConstByVal
#endif // GPOS_DEBUG
 )
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	GPOS_ASSERT(fConstByVal);
	BOOL value = false;
	if (!fConstNull)
	{
		value = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenValue, edxltokenElement);
	}
	
	return GPOS_NEW(memory_pool) CDXLDatumBool(memory_pool, pmdid, fConstNull, value);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumGeneric
//
//	@doc:
//		Parses a DXL datum of generic type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumGeneric
(
 CDXLMemoryManager *memory_manager_dxl,
 const Attributes &attrs,
 Edxltoken edxltokenElement,
 IMDId *pmdid,
 BOOL fConstNull ,
 BOOL fConstByVal
 )
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	ULONG ulPbaLength = 0;
	BYTE *pba = NULL;
	
	if (!fConstNull)
	{
		pba = GetByteArray(memory_manager_dxl, attrs, edxltokenElement, &ulPbaLength);
		if (NULL == pba)
		{
			// unable to decode value. probably not Base64 encoded.
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLInvalidAttributeValue, CDXLTokens::XmlstrToken(EdxltokenValue), CDXLTokens::GetDXLTokenStr(edxltokenElement));
		}
	}

	INT type_modifier = ExtractConvertAttrValueToInt
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeMod,
						EdxltokenScalarCast,
						true,
						IDefaultTypeModifier
						);

	return GPOS_NEW(memory_pool) CDXLDatumGeneric(memory_pool, pmdid, type_modifier, fConstByVal, fConstNull, pba, ulPbaLength);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumStatsLintMappable
//
//	@doc:
//		Parses a DXL datum of types having lint mapping
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumStatsLintMappable
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement,
	IMDId *pmdid,
	BOOL fConstNull ,
	BOOL fConstByVal
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	ULONG ulPbaLength = 0;
	BYTE *pba = NULL;

	LINT lValue = 0;
	if (!fConstNull)
	{
		pba = GetByteArray(memory_manager_dxl, attrs, edxltokenElement, &ulPbaLength);
		lValue = Value(memory_manager_dxl, attrs, edxltokenElement, pba);
	}

	INT type_modifier = ExtractConvertAttrValueToInt
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeMod,
						EdxltokenScalarCast,
						true,
						-1 /* default value */
						);

	return GPOS_NEW(memory_pool) CDXLDatumStatsLintMappable(memory_pool, pmdid, type_modifier, fConstByVal, fConstNull, pba, ulPbaLength, lValue);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::Value
//
//	@doc:
//		Return the LINT value of byte array
//
//---------------------------------------------------------------------------
LINT
CDXLOperatorFactory::Value
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement,
	BYTE *pba
	)
{
	if (NULL == pba)
	{
		// unable to decode value. probably not Base64 encoded.
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLInvalidAttributeValue, CDXLTokens::XmlstrToken(EdxltokenValue), CDXLTokens::GetDXLTokenStr(edxltokenElement));
	}

	return LValueFromAttrs(memory_manager_dxl, attrs, EdxltokenLintValue, edxltokenElement);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetByteArray
//
//	@doc:
//		Parses a byte array representation of the datum
//
//---------------------------------------------------------------------------
BYTE *
CDXLOperatorFactory::GetByteArray
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement,
	ULONG *pulLength
	)
{
	const XMLCh *xmlszValue  = ExtractAttrValue(attrs, EdxltokenValue, edxltokenElement);

	return CDXLUtils::CreateStringFrom64XMLStr(memory_manager_dxl, xmlszValue, pulLength);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxldatumStatsDoubleMappable
//
//	@doc:
//		Parses a DXL datum of types that need double mapping
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::PdxldatumStatsDoubleMappable
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenElement,
	IMDId *pmdid,
	BOOL fConstNull ,
	BOOL fConstByVal
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	ULONG ulPbaLength = 0;
	BYTE *pba = NULL;
	CDouble dValue = 0;
	
	if (!fConstNull)
	{
		pba = GetByteArray(memory_manager_dxl, attrs, edxltokenElement, &ulPbaLength);
		
		if (NULL == pba)
		{
			// unable to decode value. probably not Base64 encoded.
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLInvalidAttributeValue, CDXLTokens::XmlstrToken(EdxltokenValue), CDXLTokens::GetDXLTokenStr(edxltokenElement));
		}

		dValue = DValueFromAttrs(memory_manager_dxl, attrs, EdxltokenDoubleValue, edxltokenElement);
	}
	INT type_modifier = ExtractConvertAttrValueToInt
						(
						memory_manager_dxl,
						attrs,
						EdxltokenTypeMod,
						EdxltokenScalarCast,
						true,
						-1 /* default value */
						);
	return GPOS_NEW(memory_pool) CDXLDatumStatsDoubleMappable(memory_pool, pmdid, type_modifier, fConstByVal, fConstNull, pba, ulPbaLength, dValue);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdrgpulFromAttrs
//
//	@doc:
//		Parse a comma-separated list of unsigned long integers ids into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
ULongPtrArray *
CDXLOperatorFactory::PdrgpulFromAttrs
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	const XMLCh *xmlsz = CDXLOperatorFactory::ExtractAttrValue(attrs, edxltokenAttr, edxltokenElement);

	return PdrgpulFromXMLCh(memory_manager_dxl, xmlsz, edxltokenAttr, edxltokenElement);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdrgpmdidFromXMLCh
//
//	@doc:
//		Parse a comma-separated list of MDids into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
DrgPmdid *
CDXLOperatorFactory::PdrgpmdidFromXMLCh
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszUlList,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	DrgPmdid *mdid_array = GPOS_NEW(memory_pool) DrgPmdid(memory_pool);

	XMLStringTokenizer xmlsztok(xmlszUlList, CDXLTokens::XmlstrToken(EdxltokenComma));
	const ULONG ulNumTokens = xmlsztok.countTokens();

	for (ULONG ul = 0; ul < ulNumTokens; ul++)
	{
		XMLCh *xmlszMdid = xmlsztok.nextToken();
		GPOS_ASSERT(NULL != xmlszMdid);

		IMDId *pmdid = PmdidFromXMLCh(memory_manager_dxl, xmlszMdid, edxltokenAttr, edxltokenElement);
		mdid_array->Append(pmdid);
	}

	return mdid_array;
}

// Parse a comma-separated list of CHAR partition types into a dynamic array.
// Will raise an exception if list is not well-formed
CharPtrArray *
CDXLOperatorFactory::PdrgpszFromXMLCh
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	return PdrgptFromXMLCh<CHAR, CleanupDelete, ConvertAttrValueToChar>
			(
			memory_manager_dxl,
			xmlsz,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdrgpdrgpulFromXMLCh
//
//	@doc:
//		Parse a semicolon-separated list of comma-separated unsigned long 
//		integers into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
ULongPtrArray2D *
CDXLOperatorFactory::PdrgpdrgpulFromXMLCh
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
		
	ULongPtrArray2D *ulong_ptr_array_2D = GPOS_NEW(memory_pool) ULongPtrArray2D(memory_pool);
	
	XMLStringTokenizer xmlsztok(xmlsz, CDXLTokens::XmlstrToken(EdxltokenSemicolon));
	const ULONG ulNumTokens = xmlsztok.countTokens();
	
	for (ULONG ul = 0; ul < ulNumTokens; ul++)
	{
		XMLCh *xmlszList = xmlsztok.nextToken();
		
		GPOS_ASSERT(NULL != xmlszList);
		
		ULongPtrArray *pdrgpul = PdrgpulFromXMLCh(memory_manager_dxl, xmlszList, edxltokenAttr, edxltokenElement);
		ulong_ptr_array_2D->Append(pdrgpul);
	}
	
	return ulong_ptr_array_2D;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdrgpiParseSegmentIdList
//
//	@doc:
//		Parse a comma-separated list of segment ids into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
IntPtrArray *
CDXLOperatorFactory::PdrgpiParseSegmentIdList
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszSegIdList,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	GPOS_ASSERT(NULL != xmlszSegIdList);
	
	IntPtrArray *pdrgpiSegIds = GPOS_NEW(memory_pool) IntPtrArray(memory_pool);
	
	XMLStringTokenizer xmlsztok(xmlszSegIdList, CDXLTokens::XmlstrToken(EdxltokenComma));
	
	const ULONG ulNumSegments = xmlsztok.countTokens();
	GPOS_ASSERT(0 < ulNumSegments);
	
	for (ULONG ul = 0; ul < ulNumSegments; ul++)
	{
		XMLCh *xmlszSegId = xmlsztok.nextToken();
		
		GPOS_ASSERT(NULL != xmlszSegId);
		
		INT *piSegId = GPOS_NEW(memory_pool) INT(ConvertAttrValueToInt(memory_manager_dxl, xmlszSegId, edxltokenAttr, edxltokenElement));
		pdrgpiSegIds->Append(piSegId);
	}
	
	return pdrgpiSegIds;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdrgPstrFromXMLCh
//
//	@doc:
//		Parse a semicolon-separated list of strings into a dynamic array.
//
//---------------------------------------------------------------------------
StringPtrArray *
CDXLOperatorFactory::PdrgPstrFromXMLCh
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlsz
	)
{
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	StringPtrArray *pdrgpstr = GPOS_NEW(memory_pool) StringPtrArray(memory_pool);

	XMLStringTokenizer xmlsztok(xmlsz, CDXLTokens::XmlstrToken(EdxltokenSemicolon));
	const ULONG ulNumTokens = xmlsztok.countTokens();

	for (ULONG ul = 0; ul < ulNumTokens; ul++)
	{
		XMLCh *xmlszString = xmlsztok.nextToken();
		GPOS_ASSERT(NULL != xmlszString);

		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszString);
		pdrgpstr->Append(pstr);
	}

	return pdrgpstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::SetSegmentInfo
//
//	@doc:
//		Parses the input and output segment ids from Xerces attributes and
//		stores them in the provided DXL Motion operator.
//		Will raise an exception if lists are not well-formed 
//
//---------------------------------------------------------------------------
void
CDXLOperatorFactory::SetSegmentInfo
	(
	CDXLMemoryManager *memory_manager_dxl,
	CDXLPhysicalMotion *pdxlopMotion,
	const Attributes &attrs,
	Edxltoken edxltokenElement
	)
{
	const XMLCh *xmlszInputSegList = ExtractAttrValue
										(
										attrs,
										EdxltokenInputSegments,
										edxltokenElement
										);
	IntPtrArray *pdrgpiInputSegments = PdrgpiParseSegmentIdList
									(
									memory_manager_dxl,
									xmlszInputSegList,
									EdxltokenInputSegments,
									edxltokenElement
									);
	pdxlopMotion->SetInputSegIds(pdrgpiInputSegments);

	const XMLCh *xmlszOutputSegList = ExtractAttrValue
										(
										attrs,
										EdxltokenOutputSegments,
										edxltokenElement
										);
	IntPtrArray *pdrgpiOutputSegments = PdrgpiParseSegmentIdList
									(
									memory_manager_dxl,
									xmlszOutputSegList,
									EdxltokenOutputSegments,
									edxltokenElement
									);
	pdxlopMotion->SetOutputSegIds(pdrgpiOutputSegments);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::EdxljtParseJoinType
//
//	@doc:
//		Parse a join type from the attribute value.
//		Raise an exception if join type value is invalid.
//
//---------------------------------------------------------------------------
EdxlJoinType
CDXLOperatorFactory::EdxljtParseJoinType
	(
	const XMLCh *xmlszJoinType,
	const CWStringConst *pstrJoinName
	)
{
	EdxlJoinType join_type = EdxljtSentinel;
	
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinInner),
			xmlszJoinType))
	{
		join_type = EdxljtInner;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinLeft),
					xmlszJoinType))
	{
		join_type = EdxljtLeft;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinFull),
			xmlszJoinType))
	{
		join_type = EdxljtFull;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinRight),
				xmlszJoinType))
	{
		join_type = EdxljtRight;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinIn),
					xmlszJoinType))
	{
		join_type = EdxljtIn;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinLeftAntiSemiJoin),
					xmlszJoinType))
	{
		join_type = EdxljtLeftAntiSemijoin;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinLeftAntiSemiJoinNotIn),
					xmlszJoinType))
	{
		join_type = EdxljtLeftAntiSemijoinNotIn;
	}
	else
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenJoinType)->GetBuffer(),
			pstrJoinName->GetBuffer()
			);		
	}
	
	return join_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::EdxljtParseIndexScanDirection
//
//	@doc:
//		Parse the index scan direction from the attribute value. Raise
//		exception if it is invalid
//
//---------------------------------------------------------------------------
EdxlIndexScanDirection
CDXLOperatorFactory::EdxljtParseIndexScanDirection
	(
	const XMLCh *xmlszIndexScanDirection,
	const CWStringConst *pstrIndexScan
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexScanDirectionBackward),
			xmlszIndexScanDirection))
	{
		return EdxlisdBackward;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexScanDirectionForward),
					xmlszIndexScanDirection))
	{
		return EdxlisdForward;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexScanDirectionNoMovement),
			xmlszIndexScanDirection))
	{
		return EdxlisdNoMovement;
	}
	else
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenIndexScanDirection)->GetBuffer(),
			pstrIndexScan->GetBuffer()
			);
	}

	return EdxlisdSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxlopLogicalJoin
//
//	@doc:
//		Construct a logical join operator
//
//---------------------------------------------------------------------------
CDXLLogical*
CDXLOperatorFactory::PdxlopLogicalJoin
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	const XMLCh *xmlszJoinType = ExtractAttrValue(attrs, EdxltokenJoinType, EdxltokenLogicalJoin);
	EdxlJoinType join_type = EdxljtParseJoinType(xmlszJoinType, CDXLTokens::GetDXLTokenStr(EdxltokenLogicalJoin));

	return GPOS_NEW(memory_pool) CDXLLogicalJoin(memory_pool, join_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::DValueFromXmlstr
//
//	@doc:
//	  Converts the attribute value to CDouble
//
//---------------------------------------------------------------------------
CDouble
CDXLOperatorFactory::DValueFromXmlstr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken ,//edxltokenAttr,
	Edxltoken //edxltokenElement
	)
{
	GPOS_ASSERT(xmlszAttributeVal != NULL);
	CHAR *sz = XMLString::transcode(xmlszAttributeVal, memory_manager_dxl);

	CDouble dValue(clib::StrToD(sz));

	XMLString::release(&sz, memory_manager_dxl);
	return dValue;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::DValueFromAttrs
//
//	@doc:
//	  Extracts the value for the given attribute and converts it into CDouble
//
//---------------------------------------------------------------------------
CDouble
CDXLOperatorFactory::DValueFromAttrs
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue
											(
											attrs,
											edxltokenAttr,
											edxltokenElement
											);
	return DValueFromXmlstr
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::LValueFromXmlstr
//
//	@doc:
//	  Converts the attribute value to LINT
//
//---------------------------------------------------------------------------
LINT
CDXLOperatorFactory::LValueFromXmlstr
	(
	CDXLMemoryManager *memory_manager_dxl,
	const XMLCh *xmlszAttributeVal,
	Edxltoken ,//edxltokenAttr,
	Edxltoken //edxltokenElement
	)
{
	GPOS_ASSERT(NULL != xmlszAttributeVal);
	CHAR *sz = XMLString::transcode(xmlszAttributeVal, memory_manager_dxl);
	CHAR *szEnd = NULL;

	LINT lValue = clib::StrToLL(sz, &szEnd, 10);
	XMLString::release(&sz, memory_manager_dxl);

	return lValue;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::LValueFromAttrs
//
//	@doc:
//	  Extracts the value for the given attribute and converts it into LINT
//
//---------------------------------------------------------------------------
LINT
CDXLOperatorFactory::LValueFromAttrs
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement,
	BOOL fOptional,
	LINT lDefaultValue
	)
{
	const XMLCh *xmlszValue = CDXLOperatorFactory::ExtractAttrValue(attrs, edxltokenAttr, edxltokenElement, fOptional);

	if (NULL == xmlszValue)
	{
		return lDefaultValue;
	}

	return LValueFromXmlstr(memory_manager_dxl, xmlszValue, edxltokenAttr, edxltokenElement);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::Sysid
//
//	@doc:
//	  Extracts the value for the given attribute and converts it into CDouble
//
//---------------------------------------------------------------------------
CSystemId
CDXLOperatorFactory::Sysid
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	// extract systemids
	const XMLCh *xmlsz = ExtractAttrValue(attrs, edxltokenAttr, edxltokenElement);

	// get sysid components
	XMLStringTokenizer xmlsztokSysid(xmlsz, CDXLTokens::XmlstrToken(EdxltokenDot));
	GPOS_ASSERT(2 == xmlsztokSysid.countTokens());
	
	XMLCh *xmlszType = xmlsztokSysid.nextToken();
	ULONG ulType = CDXLOperatorFactory::ConvertAttrValueToUlong(memory_manager_dxl, xmlszType, edxltokenAttr, edxltokenElement);
	
	XMLCh *xmlszName = xmlsztokSysid.nextToken();
	CWStringDynamic *pstrName = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszName);
	
	CSystemId sysid((IMDId::EMDIdType) ulType, pstrName->GetBuffer(), pstrName->Length());	
	GPOS_DELETE(pstrName);
	
	return sysid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::PdxlopWindowRef
//
//	@doc:
//		Construct an WindowRef operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::PdxlopWindowRef
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	IMDId *mdid_func = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenWindowrefOid, EdxltokenScalarWindowref);
	IMDId *mdid_return_type = PmdidFromAttrs(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarWindowref);
	BOOL fDistinct = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenWindowrefDistinct, EdxltokenScalarWindowref);
	BOOL fStarArg = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenWindowrefStarArg, EdxltokenScalarWindowref);
	BOOL fSimpleAgg = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenWindowrefSimpleAgg, EdxltokenScalarWindowref);
	ULONG ulWinspecPos = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenWindowrefWinSpecPos, EdxltokenScalarWindowref);

	const XMLCh *xmlszStage  = ExtractAttrValue(attrs, EdxltokenWindowrefStrategy, EdxltokenScalarWindowref);
	EdxlWinStage edxlwinstage = EdxlwinstageSentinel;

	ULONG rgrgulMapping[][2] =
					{
					{EdxlwinstageImmediate, EdxltokenWindowrefStageImmediate},
					{EdxlwinstagePreliminary, EdxltokenWindowrefStagePreliminary},
					{EdxlwinstageRowKey, EdxltokenWindowrefStageRowKey}
					};

	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		Edxltoken edxltk = (Edxltoken) pulElem[1];
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(edxltk), xmlszStage))
		{
			edxlwinstage = (EdxlWinStage) pulElem[0];
			break;
		}
	}
	GPOS_ASSERT(EdxlwinstageSentinel != edxlwinstage);

	return GPOS_NEW(memory_pool) CDXLScalarWindowRef(memory_pool, mdid_func, mdid_return_type, fDistinct, fStarArg, fSimpleAgg, edxlwinstage, ulWinspecPos);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::Ecmpt
//
//	@doc:
//		Parse comparison type
//
//---------------------------------------------------------------------------
IMDType::ECmpType
CDXLOperatorFactory::Ecmpt
	(
	const XMLCh* xmlszCmpType
	)
{
	ULONG rgrgulMapping[][2] = 
	{
	{EdxltokenCmpEq, IMDType::EcmptEq},
	{EdxltokenCmpNeq,IMDType::EcmptNEq},
	{EdxltokenCmpLt, IMDType::EcmptL},
	{EdxltokenCmpLeq, IMDType::EcmptLEq},
	{EdxltokenCmpGt, IMDType::EcmptG},
	{EdxltokenCmpGeq,IMDType::EcmptGEq}, 
	{EdxltokenCmpIDF, IMDType::EcmptIDF},
	{EdxltokenCmpOther, IMDType::EcmptOther}
	};
	
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgrgulMapping); ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		Edxltoken edxltk = (Edxltoken) pulElem[0];
		
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(edxltk),
				xmlszCmpType))
		{
			return (IMDType::ECmpType) pulElem[1];
		}
	}
	
	GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLInvalidAttributeValue, CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOpCmpType)->GetBuffer(), CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOp)->GetBuffer());		
	return (IMDType::ECmpType) IMDType::EcmptOther;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::EreldistrpolicyFromXmlstr
//
//	@doc:
//		Parse relation distribution policy from XML string
//
//---------------------------------------------------------------------------
IMDRelation::Ereldistrpolicy
CDXLOperatorFactory::EreldistrpolicyFromXmlstr
	(
	const XMLCh* xmlsz
	)
{
	GPOS_ASSERT(NULL != xmlsz);
	IMDRelation::Ereldistrpolicy rel_distr_policy = IMDRelation::EreldistrSentinel;
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelDistrMasterOnly)))
	{
		rel_distr_policy = IMDRelation::EreldistrMasterOnly;
	}
	else if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelDistrHash)))
	{
		rel_distr_policy = IMDRelation::EreldistrHash;
	}
	else if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelDistrRandom)))
	{
		rel_distr_policy = IMDRelation::EreldistrRandom;
	}
	
	return rel_distr_policy;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ErelstoragetypeFromXmlstr
//
//	@doc:
//		Parse relation storage type from XML string
//
//---------------------------------------------------------------------------
IMDRelation::Erelstoragetype
CDXLOperatorFactory::ErelstoragetypeFromXmlstr
	(
	const XMLCh* xmlsz
	)
{
	GPOS_ASSERT(NULL != xmlsz);
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelStorageHeap)))
	{
		return IMDRelation::ErelstorageHeap;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelStorageAppendOnlyCols)))
	{
		return IMDRelation::ErelstorageAppendOnlyCols;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelStorageAppendOnlyRows)))
	{
		return IMDRelation::ErelstorageAppendOnlyRows;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelStorageAppendOnlyParquet)))
	{
		return IMDRelation::ErelstorageAppendOnlyParquet;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelStorageExternal)))
	{
		return IMDRelation::ErelstorageExternal;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenRelStorageVirtual)))
	{
		return IMDRelation::ErelstorageVirtual;
	}
	
	GPOS_ASSERT(!"Unrecognized storage type");
	
	return IMDRelation::ErelstorageSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::EctascommitFromAttr
//
//	@doc:
//		Parse on commit action spec from XML attributes
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions::ECtasOnCommitAction
CDXLOperatorFactory::EctascommitFromAttr
	(
	const Attributes &attrs
	)
{
	const XMLCh *xmlsz = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenOnCommitAction));

	if (NULL == xmlsz)
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLMissingAttribute,
			CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitAction)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenCTASOptions)->GetBuffer()
			);
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenOnCommitPreserve)))
	{
		return CDXLCtasStorageOptions::EctascommitPreserve;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenOnCommitDelete)))
	{
		return CDXLCtasStorageOptions::EctascommitDelete;
	}
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenOnCommitDrop)))
	{
		return CDXLCtasStorageOptions::EctascommitDrop;
	}
	
	if (0 != XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenOnCommitNOOP)))
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitAction)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenCTASOptions)->GetBuffer()
			);	
	}
	
	return CDXLCtasStorageOptions::EctascommitNOOP;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::EmdindtFromAttr
//
//	@doc:
//		Parse index type from XML attributes
//
//---------------------------------------------------------------------------
IMDIndex::EmdindexType
CDXLOperatorFactory::EmdindtFromAttr
	(
	const Attributes &attrs
	)
{
	const XMLCh *xmlsz = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenIndexType));

	if (NULL == xmlsz || 0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenIndexTypeBtree)))
	{
		// default is btree
		return IMDIndex::EmdindBtree;
	}
	
	
	if (0 == XMLString::compareString(xmlsz, CDXLTokens::XmlstrToken(EdxltokenIndexTypeBitmap)))
	{
		return IMDIndex::EmdindBitmap;
	}
	
	GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenIndexType)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenIndex)->GetBuffer()
			);	
	
	return IMDIndex::EmdindSentinel;
}

// EOF
