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
	const XMLCh *subquery_name_xml = ExtractAttrValue
								(
								attrs,
								EdxltokenAlias,
								EdxltokenPhysicalSubqueryScan
								);

	CWStringDynamic *subquery_name_str = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl,subquery_name_xml);
	

	// create a copy of the string in the CMDName constructor
	CMDName *subquery_name = GPOS_NEW(memory_pool) CMDName(memory_pool, subquery_name_str);
	
	GPOS_DELETE(subquery_name_str);

	return GPOS_NEW(memory_pool) CDXLPhysicalSubqueryScan(memory_pool, subquery_name);
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
	
	const XMLCh *join_type_xml = ExtractAttrValue
									(
									attrs,
									EdxltokenJoinType,
									EdxltokenPhysicalHashJoin
									);
	
	EdxlJoinType join_type = ParseJoinType(join_type_xml, CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalHashJoin));
	
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
	
	const XMLCh *join_type_xml = ExtractAttrValue
									(
									attrs,
									EdxltokenJoinType,
									EdxltokenPhysicalNLJoin
									);
	
	BOOL is_index_nlj = false;
	const XMLCh *xmlszIndexNLJ = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenPhysicalNLJoinIndex));
	if (NULL != xmlszIndexNLJ)
	{
		is_index_nlj = ConvertAttrValueToBool
						(
						memory_manager_dxl,
						xmlszIndexNLJ,
						EdxltokenPhysicalNLJoinIndex,
						EdxltokenPhysicalNLJoin
						);
	}

	EdxlJoinType join_type = ParseJoinType(join_type_xml, CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalNLJoin));
	
	return GPOS_NEW(memory_pool) CDXLPhysicalNLJoin(memory_pool, join_type, is_index_nlj);
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
	
	const XMLCh *join_type_xml = ExtractAttrValue
									(
									attrs,
									EdxltokenJoinType,
									EdxltokenPhysicalMergeJoin
									);
	
	EdxlJoinType join_type = ParseJoinType(join_type_xml, CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalMergeJoin));
	
	BOOL is_unique_outer = ExtractConvertAttrValueToBool
								(
								memory_manager_dxl,
								attrs,
								EdxltokenMergeJoinUniqueOuter,
								EdxltokenPhysicalMergeJoin
								);
	
	return GPOS_NEW(memory_pool) CDXLPhysicalMergeJoin(memory_pool, join_type, is_unique_outer);
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
	
	BOOL is_duplicate_sensitive = false;
	
	const XMLCh *duplicate_sensitive_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDuplicateSensitive));
	if (NULL != duplicate_sensitive_xml)
	{
		is_duplicate_sensitive = ConvertAttrValueToBool(memory_manager_dxl, duplicate_sensitive_xml, EdxltokenDuplicateSensitive, EdxltokenPhysicalRedistributeMotion);
	}
		
	CDXLPhysicalRedistributeMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalRedistributeMotion(memory_pool, is_duplicate_sensitive);
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

	ULONG segment_col_id = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenSegmentIdCol, EdxltokenPhysicalRoutedDistributeMotion);
	
	CDXLPhysicalRoutedDistributeMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalRoutedDistributeMotion(memory_pool, segment_col_id);
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
	
	BOOL is_duplicate_sensitive = false;

	const XMLCh *duplicate_sensitive_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDuplicateSensitive));
	if (NULL != duplicate_sensitive_xml)
	{
		is_duplicate_sensitive = ConvertAttrValueToBool(memory_manager_dxl, duplicate_sensitive_xml, EdxltokenDuplicateSensitive, EdxltokenPhysicalRandomMotion);
	}

	CDXLPhysicalRandomMotion *dxl_op = GPOS_NEW(memory_pool) CDXLPhysicalRandomMotion(memory_pool, is_duplicate_sensitive);
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
	
	BOOL is_target = ExtractConvertAttrValueToBool
						(
						memory_manager_dxl,
						attrs,
						EdxltokenAppendIsTarget,
						EdxltokenPhysicalAppend
						);
	
	BOOL is_zapped = ExtractConvertAttrValueToBool
						(
						memory_manager_dxl,
						attrs,
						EdxltokenAppendIsZapped,
						EdxltokenPhysicalAppend
						);

	return GPOS_NEW(memory_pool) CDXLPhysicalAppend(memory_pool, is_target, is_zapped);
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
	
	const XMLCh *agg_strategy_xml = ExtractAttrValue
										(
										attrs,
										EdxltokenAggStrategy,
										EdxltokenPhysicalAggregate
										);
	
	EdxlAggStrategy agg_strategy_dxl = EdxlaggstrategySentinel;
	
	if (0 == XMLString::compareString
							(
							CDXLTokens::XmlstrToken(EdxltokenAggStrategyPlain),
							agg_strategy_xml
							))
	{
		agg_strategy_dxl = EdxlaggstrategyPlain;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggStrategySorted),
									agg_strategy_xml))
	{
		agg_strategy_dxl = EdxlaggstrategySorted;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenAggStrategyHashed),
									agg_strategy_xml))
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

	const XMLCh *stream_safe_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenAggStreamSafe));
	if (NULL != stream_safe_xml)
	{
		stream_safe = ConvertAttrValueToBool
						(
						memory_manager_dxl,
						stream_safe_xml,
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
	BOOL discard_duplicates = ExtractConvertAttrValueToBool
								(
								memory_manager_dxl,
								attrs,
								EdxltokenSortDiscardDuplicates,
								EdxltokenPhysicalSort
								);
	
	return GPOS_NEW(memory_pool) CDXLPhysicalSort(memory_pool, discard_duplicates);
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
	
	CDXLPhysicalMaterialize *materialize_dxlnode = NULL;
	
	// is this a multi-slice spool
	BOOL eager_materialize = ExtractConvertAttrValueToBool
								(
								memory_manager_dxl,
								attrs,
								EdxltokenMaterializeEager,
								EdxltokenPhysicalMaterialize
								);
	
	if (1 == attrs.getLength())
	{
		// no spooling info specified -> create a non-spooling materialize operator
		materialize_dxlnode = GPOS_NEW(memory_pool) CDXLPhysicalMaterialize(memory_pool, eager_materialize);
	}
	else
	{
		// parse spool id
		ULONG spool_id = ExtractConvertAttrValueToUlong
							(
							memory_manager_dxl,
							attrs,
							EdxltokenSpoolId,
							EdxltokenPhysicalMaterialize
							);
	
		// parse id of executor slice
		INT executor_slice = ExtractConvertAttrValueToInt
									(
									memory_manager_dxl,
									attrs,
									EdxltokenExecutorSliceId,
									EdxltokenPhysicalMaterialize
									);
		
		ULONG num_consumer_slices = ExtractConvertAttrValueToUlong
									(
									memory_manager_dxl,
									attrs,
									EdxltokenConsumerSliceCount,
									EdxltokenPhysicalMaterialize
									);
	
		materialize_dxlnode = GPOS_NEW(memory_pool) CDXLPhysicalMaterialize(memory_pool, eager_materialize, spool_id, executor_slice, num_consumer_slices);
	}
	
	return materialize_dxlnode;
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
	const XMLCh *scalar_cmp_xml = ExtractAttrValue(attrs, EdxltokenComparisonOp, EdxltokenScalarComp);
	
	// parse op no and function id
	IMDId *op_id = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenOpNo, EdxltokenScalarComp);
	
	// parse comparison operator from string
	CWStringDynamic *comp_op_name = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, scalar_cmp_xml);
	
	// copy dynamic string into const string
	CWStringConst *comp_op_name_copy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, comp_op_name->GetBuffer());

	// cleanup
	GPOS_DELETE(comp_op_name);
	
	return GPOS_NEW(memory_pool) CDXLScalarComp(memory_pool, op_id, comp_op_name_copy);
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
	IMDId *op_id = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenOpNo,EdxltokenScalarDistinctComp);

	return GPOS_NEW(memory_pool) CDXLScalarDistinctComp(memory_pool, op_id);
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
	const XMLCh *op_expr_xml = ExtractAttrValue
									(
									attrs,
									EdxltokenOpName,
									EdxltokenScalarOpExpr
									);

	IMDId *op_id = ExtractConvertAttrValueToMdId
							(
							memory_manager_dxl,
							attrs,
							EdxltokenOpNo,
							EdxltokenScalarOpExpr
							);

	IMDId *return_type_mdid = NULL;
	const XMLCh *return_type_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenOpType));

	if (NULL != return_type_xml)
	{
		return_type_mdid = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenOpType, EdxltokenScalarOpExpr);
	}
	
	CWStringDynamic *value = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, op_expr_xml);
	CWStringConst *value_copy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, value->GetBuffer());
	GPOS_DELETE(value);

	return GPOS_NEW(memory_pool) CDXLScalarOpExpr(memory_pool, op_id, return_type_mdid, value_copy);
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
	const XMLCh *op_expr_xml = ExtractAttrValue
								(
								attrs,
								EdxltokenOpName,
								EdxltokenScalarArrayComp
								);
	
	const XMLCh *op_type_xml = ExtractAttrValue
									(
									attrs,
									EdxltokenOpType,
									EdxltokenScalarArrayComp
									);

	// parse operator no and function id
	IMDId *op_id = ExtractConvertAttrValueToMdId
							(
							memory_manager_dxl,
							attrs,
							EdxltokenOpNo,
							EdxltokenScalarArrayComp
							);

	EdxlArrayComparisonType array_comp_type = Edxlarraycomparisontypeany;

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpTypeAll), op_type_xml))
	{
		array_comp_type = Edxlarraycomparisontypeall;
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpTypeAny), op_type_xml))
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLInvalidAttributeValue,
			CDXLTokens::GetDXLTokenStr(EdxltokenOpType)->GetBuffer(),
			CDXLTokens::GetDXLTokenStr(EdxltokenScalarArrayComp)->GetBuffer()
			);
	}

	CWStringDynamic *opname = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, op_expr_xml);
	CWStringConst *opname_copy = GPOS_NEW(memory_pool) CWStringConst(memory_pool, opname->GetBuffer());
	GPOS_DELETE(opname);

	return GPOS_NEW(memory_pool) CDXLScalarArrayComp(memory_pool, op_id, opname_copy, array_comp_type);
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
	
	return GPOS_NEW(memory_pool) CDXLScalarBoolExpr(memory_pool, edxlboolexprType);
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
	DrgPdxlcr *dxl_colref_array,
	EdxlSubPlanType dxl_subplan_type,
	CDXLNode *dxlnode_test_expr
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	return GPOS_NEW(memory_pool) CDXLScalarSubPlan(memory_pool, pmdid, dxl_colref_array, dxl_subplan_type, dxlnode_test_expr);
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
	const BOOL is_null
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	
	return GPOS_NEW(memory_pool) CDXLScalarNullTest(memory_pool, is_null);
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
	IMDId *mdid_type = ExtractConvertAttrValueToMdId
							(
							memory_manager_dxl,
							attrs,
							EdxltokenTypeId,
							EdxltokenScalarCast
							);

	IMDId *mdid_func = ExtractConvertAttrValueToMdId
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
	IMDId *mdid_type = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarCoerceToDomain);
	INT type_modifier = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenTypeMod, EdxltokenScalarCoerceToDomain, true, IDefaultTypeModifier);
	ULONG coercion_form = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenCoercionForm, EdxltokenScalarCoerceToDomain);
	INT location = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenLocation, EdxltokenScalarCoerceToDomain);

	return GPOS_NEW(memory_pool) CDXLScalarCoerceToDomain(memory_pool, mdid_type, type_modifier, (EdxlCoercionForm) coercion_form, location);
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
	IMDId *mdid_type = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarCoerceViaIO);
	INT type_modifier = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenTypeMod, EdxltokenScalarCoerceViaIO, true, IDefaultTypeModifier);
	ULONG coercion_form = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenCoercionForm, EdxltokenScalarCoerceViaIO);
	INT location = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenLocation, EdxltokenScalarCoerceViaIO);

	return GPOS_NEW(memory_pool) CDXLScalarCoerceViaIO(memory_pool, mdid_type, type_modifier, (EdxlCoercionForm) coercion_form, location);
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

	IMDId *pmdidElementFunc = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenElementFunc, EdxltokenScalarArrayCoerceExpr);
	IMDId *mdid_type = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarArrayCoerceExpr);
	INT type_modifier = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenTypeMod, EdxltokenScalarArrayCoerceExpr, true, IDefaultTypeModifier);
	BOOL fIsExplicit = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenIsExplicit, EdxltokenScalarArrayCoerceExpr);
	ULONG coercion_form = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenCoercionForm, EdxltokenScalarArrayCoerceExpr);
	INT location = ExtractConvertAttrValueToInt(memory_manager_dxl, attrs, EdxltokenLocation, EdxltokenScalarArrayCoerceExpr);

	return GPOS_NEW(memory_pool) CDXLScalarArrayCoerceExpr(memory_pool, pmdidElementFunc, mdid_type, type_modifier, fIsExplicit, (EdxlCoercionForm) coercion_form, location);
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
	IMDId *mdid_type = ExtractConvertAttrValueToMdId
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
	
	IMDId *mdid_func = ExtractConvertAttrValueToMdId
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

	IMDId *mdid_return_type = ExtractConvertAttrValueToMdId
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
	
	IMDId *pmdidAgg = ExtractConvertAttrValueToMdId
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
	const XMLCh *return_type_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenTypeId));
	if (NULL != return_type_xml)
	{
		pmdidResolvedRetType = ExtractConvertAttrValueToMdId
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

	EdxlFrameBoundary dxl_frame_boundary = EdxlfbSentinel;
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

	const ULONG arity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		Edxltoken edxltk = (Edxltoken) pulElem[1];
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(edxltk), xmlszBoundary))
		{
			dxl_frame_boundary = (EdxlFrameBoundary) pulElem[0];
			break;
		}
	}

	if (EdxlfbSentinel == dxl_frame_boundary)
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

	return dxl_frame_boundary;
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

	EdxlFrameSpec dxl_frame_boundary = EdxlfsSentinel;
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFSRow), xmlszfs))
	{
		dxl_frame_boundary = EdxlfsRow;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFSRange), xmlszfs))
	{
		dxl_frame_boundary = EdxlfsRange;
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

	return dxl_frame_boundary;
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
	const ULONG arity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < arity; ul++)
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
	
	IMDId *elem_type_mdid = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenArrayElementType, EdxltokenScalarArray);
	IMDId *array_type_mdid = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenArrayType, EdxltokenScalarArray);
	BOOL fMultiDimensional = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenArrayMultiDim, EdxltokenScalarArray);

	return GPOS_NEW(memory_pool) CDXLScalarArray(memory_pool, elem_type_mdid, array_type_mdid, fMultiDimensional);
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
	
	CDXLColRef *dxl_colref = MakeDXLColRef(memory_manager_dxl, attrs, EdxltokenScalarIdent);

	return GPOS_NEW(memory_pool) CDXLScalarIdent(memory_pool, dxl_colref);
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
	ULONG id = ExtractConvertAttrValueToUlong
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
	
	return GPOS_NEW(memory_pool) CDXLScalarProjElem(memory_pool, id, mdname);
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

	IMDId *mdid_type = ExtractConvertAttrValueToMdId
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
	IMDId *pmdidSortOp = ExtractConvertAttrValueToMdId
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
	IMDId *pmdid = ExtractConvertAttrValueToMdId
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
	IMDId *pmdid = ExtractConvertAttrValueToMdId
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
	ULONG id = ExtractConvertAttrValueToUlong
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
	IMDId *mdid_type = ExtractConvertAttrValueToMdId
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
	
	return GPOS_NEW(memory_pool) CDXLColDescr(memory_pool, mdname, id, iAttno, mdid_type, type_modifier, fColDropped, ulColLen);
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
	ULONG id = 0;
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
	
	id = XMLString::parseInt(xmlszColumnId, memory_manager_dxl);
		
	CWStringDynamic *pstrColumnName =  CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl,xmlszColumnName);

	// create a copy of the string in the CMDName constructor
	CMDName *mdname = GPOS_NEW(memory_pool) CMDName(memory_pool, pstrColumnName);
	
	GPOS_DELETE(pstrColumnName);

	IMDId *mdid_type = ExtractConvertAttrValueToMdId
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
	
	return GPOS_NEW(memory_pool) CDXLColRef(memory_pool, mdname, id, mdid_type, type_modifier);
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
	INT segment_id = -1;
	try
	{
		segment_id = XMLString::parseInt(xmlszSegId, memory_manager_dxl);
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

	return segment_id;
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
	ULONG id = 0;
	try
	{
		id = XMLString::parseInt(xmlszAttributeVal, memory_manager_dxl);
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
	return id;
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
//		CDXLOperatorFactory::ExtractConvertAttrValueToMdId
//
//	@doc:
//		Parse a metadata id object from the XML attributes of the specified element.
//
//---------------------------------------------------------------------------
IMDId *
CDXLOperatorFactory::ExtractConvertAttrValueToMdId
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
	
	return MakeMdIdFromStr(memory_manager_dxl, xmlszMdid, edxltokenAttr, edxltokenElement);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeMdIdFromStr
//
//	@doc:
//		Parse a metadata id object from the XML attributes of the specified element.
//
//---------------------------------------------------------------------------
IMDId *
CDXLOperatorFactory::MakeMdIdFromStr
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
			pmdid = GetGPDBMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidGPDBCtas:
			pmdid = GetGPDBCTASMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidColStats:
			pmdid = GetColStatsMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidRelStats:
			pmdid = GetRelStatsMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidCastFunc:
			pmdid = GetCastFuncMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		case IMDId::EmdidScCmp:
			pmdid = GetScCmpMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
			break;
			
		default:
			GPOS_ASSERT(!"Unrecognized mdid type");		
	}
	
	pdrgpxmlsz->Release();
	
	return pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetGPDBMdId
//
//	@doc:
//		Construct a GPDB mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdGPDB *
CDXLOperatorFactory::GetGPDBMdId
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
//		CDXLOperatorFactory::GetGPDBCTASMdId
//
//	@doc:
//		Construct a GPDB CTAS mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdGPDB *
CDXLOperatorFactory::GetGPDBCTASMdId
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
//		CDXLOperatorFactory::GetColStatsMdId
//
//	@doc:
//		Construct a column stats mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdColStats *
CDXLOperatorFactory::GetColStatsMdId
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(GPDXL_GPDB_MDID_COMPONENTS + 1 == pdrgpxmlsz->Size());

	CMDIdGPDB *rel_mdid = GetGPDBMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	
	XMLCh *xmlszAttno = (*pdrgpxmlsz)[3];
	ULONG ulAttno = ConvertAttrValueToUlong(memory_manager_dxl, xmlszAttno, edxltokenAttr, edxltokenElement);

	// construct metadata id object
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdColStats(rel_mdid, ulAttno);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetRelStatsMdId
//
//	@doc:
//		Construct a relation stats mdid from an array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdRelStats *
CDXLOperatorFactory::GetRelStatsMdId
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(GPDXL_GPDB_MDID_COMPONENTS == pdrgpxmlsz->Size());

	CMDIdGPDB *rel_mdid = GetGPDBMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	
	// construct metadata id object
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdRelStats(rel_mdid);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetCastFuncMdId
//
//	@doc:
//		Construct a cast function mdid from the array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdCast *
CDXLOperatorFactory::GetCastFuncMdId
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(2 * GPDXL_GPDB_MDID_COMPONENTS == pdrgpxmlsz->Size());

	CMDIdGPDB *mdid_src = GetGPDBMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	DrgPxmlsz *pdrgpxmlszDest = GPOS_NEW(memory_manager_dxl->Pmp()) DrgPxmlsz(memory_manager_dxl->Pmp());
	
	for (ULONG ul = GPDXL_GPDB_MDID_COMPONENTS; ul < GPDXL_GPDB_MDID_COMPONENTS * 2; ul++)
	{
		pdrgpxmlszDest->Append((*pdrgpxmlsz)[ul]);
	}
	
	CMDIdGPDB *mdid_dest = GetGPDBMdId(memory_manager_dxl, pdrgpxmlszDest, edxltokenAttr, edxltokenElement);
	pdrgpxmlszDest->Release();
	
	return GPOS_NEW(memory_manager_dxl->Pmp()) CMDIdCast(mdid_src, mdid_dest);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetScCmpMdId
//
//	@doc:
//		Construct a scalar comparison operator mdid from the array of XML string components.
//
//---------------------------------------------------------------------------
CMDIdScCmp *
CDXLOperatorFactory::GetScCmpMdId
	(
	CDXLMemoryManager *memory_manager_dxl,
	DrgPxmlsz *pdrgpxmlsz,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{
	GPOS_ASSERT(2 * GPDXL_GPDB_MDID_COMPONENTS + 1 == pdrgpxmlsz->Size());

	CMDIdGPDB *pmdidLeft = GetGPDBMdId(memory_manager_dxl, pdrgpxmlsz, edxltokenAttr, edxltokenElement);
	DrgPxmlsz *pdrgpxmlszRight = GPOS_NEW(memory_manager_dxl->Pmp()) DrgPxmlsz(memory_manager_dxl->Pmp());
	
	for (ULONG ul = GPDXL_GPDB_MDID_COMPONENTS; ul < GPDXL_GPDB_MDID_COMPONENTS * 2 + 1; ul++)
	{
		pdrgpxmlszRight->Append((*pdrgpxmlsz)[ul]);
	}
	
	CMDIdGPDB *pmdidRight = GetGPDBMdId(memory_manager_dxl, pdrgpxmlszRight, edxltokenAttr, edxltokenElement);
	
	// parse the comparison type from the last component of the mdid
	XMLCh *xml_str_comp_type = (*pdrgpxmlszRight)[pdrgpxmlszRight->Size() - 1];
	IMDType::ECmpType ecmpt = (IMDType::ECmpType) ConvertAttrValueToUlong(memory_manager_dxl, xml_str_comp_type, edxltokenAttr, edxltokenElement);
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
	IMDId *pmdid = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenTypeId, edxltokenElement);
	GPOS_ASSERT(IMDId::EmdidGPDB == pmdid->Emdidt());
	CMDIdGPDB *pmdidgpdbd = CMDIdGPDB::PmdidConvert(pmdid);

	// get the type id from string
	BOOL fConstNull = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenIsNull, edxltokenElement);
	BOOL fConstByVal = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenIsByValue, edxltokenElement);

	
	SDXLDatumFactoryElem rgTranslators[] =
	{
		// native support
		{CMDIdGPDB::m_mdidInt2.OidObjectId() , &CDXLOperatorFactory::GetDatumInt2},
		{CMDIdGPDB::m_mdidInt4.OidObjectId() , &CDXLOperatorFactory::GetDatumInt4},
		{CMDIdGPDB::m_mdidInt8.OidObjectId() , &CDXLOperatorFactory::PdxldatumInt8},
		{CMDIdGPDB::m_mdidBool.OidObjectId() , &CDXLOperatorFactory::GetDatumBool},
		{CMDIdGPDB::m_mdidOid.OidObjectId() , &CDXLOperatorFactory::GetDatumOid},
		// types with long int mapping
		{CMDIdGPDB::m_mdidBPChar.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsLintMappable},
		{CMDIdGPDB::m_mdidVarChar.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsLintMappable},
		{CMDIdGPDB::m_mdidText.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsLintMappable},
		{CMDIdGPDB::m_mdidCash.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsLintMappable},
		// non-integer numeric types
		{CMDIdGPDB::m_mdidNumeric.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidFloat4.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidFloat8.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		// network-related types
		{CMDIdGPDB::m_mdidInet.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidCidr.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidMacaddr.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		// time-related types
		{CMDIdGPDB::m_mdidDate.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTime.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimeTz.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimestamp.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimestampTz.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidAbsTime.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidRelTime.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidInterval.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable},
		{CMDIdGPDB::m_mdidTimeInterval.OidObjectId(), &CDXLOperatorFactory::GetDatumStatsDoubleMappable}
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
//		CDXLOperatorFactory::GetDatumOid
//
//	@doc:
//		Parses a DXL datum of oid type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumOid
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
//		CDXLOperatorFactory::GetDatumInt2
//
//	@doc:
//		Parses a DXL datum of int2 type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumInt2
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
//		CDXLOperatorFactory::GetDatumInt4
//
//	@doc:
//		Parses a DXL datum of int4 type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumInt4
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
		lVal = ExtractConvertAttrValueToLint(memory_manager_dxl, attrs, EdxltokenValue, edxltokenElement);
	}
	
	return GPOS_NEW(memory_pool) CDXLDatumInt8(memory_pool, pmdid, fConstNull, lVal);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::GetDatumBool
//
//	@doc:
//		Parses a DXL datum of boolean type
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumBool
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
//		CDXLOperatorFactory::GetDatumStatsLintMappable
//
//	@doc:
//		Parses a DXL datum of types having lint mapping
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumStatsLintMappable
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

	return ExtractConvertAttrValueToLint(memory_manager_dxl, attrs, EdxltokenLintValue, edxltokenElement);
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
//		CDXLOperatorFactory::GetDatumStatsDoubleMappable
//
//	@doc:
//		Parses a DXL datum of types that need double mapping
//
//---------------------------------------------------------------------------
CDXLDatum *
CDXLOperatorFactory::GetDatumStatsDoubleMappable
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

		dValue = ExtractConvertAttrValueToDouble(memory_manager_dxl, attrs, EdxltokenDoubleValue, edxltokenElement);
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
//		CDXLOperatorFactory::ExtractConvertValuesToArray
//
//	@doc:
//		Parse a comma-separated list of unsigned long integers ids into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
ULongPtrArray *
CDXLOperatorFactory::ExtractConvertValuesToArray
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
//		CDXLOperatorFactory::ExtractConvertMdIdsToArray
//
//	@doc:
//		Parse a comma-separated list of MDids into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
DrgPmdid *
CDXLOperatorFactory::ExtractConvertMdIdsToArray
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

		IMDId *pmdid = MakeMdIdFromStr(memory_manager_dxl, xmlszMdid, edxltokenAttr, edxltokenElement);
		mdid_array->Append(pmdid);
	}

	return mdid_array;
}

// Parse a comma-separated list of CHAR partition types into a dynamic array.
// Will raise an exception if list is not well-formed
CharPtrArray *
CDXLOperatorFactory::ExtractConvertPartitionTypeToArray
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
//		CDXLOperatorFactory::ExtractConvertUlongTo2DArray
//
//	@doc:
//		Parse a semicolon-separated list of comma-separated unsigned long 
//		integers into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
ULongPtrArray2D *
CDXLOperatorFactory::ExtractConvertUlongTo2DArray
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
//		CDXLOperatorFactory::ExtractConvertSegmentIdsToArray
//
//	@doc:
//		Parse a comma-separated list of segment ids into a dynamic array.
//		Will raise an exception if list is not well-formed
//
//---------------------------------------------------------------------------
IntPtrArray *
CDXLOperatorFactory::ExtractConvertSegmentIdsToArray
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
	
	const ULONG num_of_segments = xmlsztok.countTokens();
	GPOS_ASSERT(0 < num_of_segments);
	
	for (ULONG ul = 0; ul < num_of_segments; ul++)
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
//		CDXLOperatorFactory::ExtractConvertStrsToArray
//
//	@doc:
//		Parse a semicolon-separated list of strings into a dynamic array.
//
//---------------------------------------------------------------------------
StringPtrArray *
CDXLOperatorFactory::ExtractConvertStrsToArray
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

		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xmlszString);
		pdrgpstr->Append(str);
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
	IntPtrArray *pdrgpiInputSegments = ExtractConvertSegmentIdsToArray
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
	IntPtrArray *pdrgpiOutputSegments = ExtractConvertSegmentIdsToArray
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
//		CDXLOperatorFactory::ParseJoinType
//
//	@doc:
//		Parse a join type from the attribute value.
//		Raise an exception if join type value is invalid.
//
//---------------------------------------------------------------------------
EdxlJoinType
CDXLOperatorFactory::ParseJoinType
	(
	const XMLCh *join_type_xml,
	const CWStringConst *pstrJoinName
	)
{
	EdxlJoinType join_type = EdxljtSentinel;
	
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinInner),
			join_type_xml))
	{
		join_type = EdxljtInner;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinLeft),
					join_type_xml))
	{
		join_type = EdxljtLeft;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinFull),
			join_type_xml))
	{
		join_type = EdxljtFull;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinRight),
				join_type_xml))
	{
		join_type = EdxljtRight;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinIn),
					join_type_xml))
	{
		join_type = EdxljtIn;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinLeftAntiSemiJoin),
					join_type_xml))
	{
		join_type = EdxljtLeftAntiSemijoin;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenJoinLeftAntiSemiJoinNotIn),
					join_type_xml))
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
//		CDXLOperatorFactory::ParseIndexScanDirection
//
//	@doc:
//		Parse the index scan direction from the attribute value. Raise
//		exception if it is invalid
//
//---------------------------------------------------------------------------
EdxlIndexScanDirection
CDXLOperatorFactory::ParseIndexScanDirection
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
//		CDXLOperatorFactory::MakeLogicalJoin
//
//	@doc:
//		Construct a logical join operator
//
//---------------------------------------------------------------------------
CDXLLogical*
CDXLOperatorFactory::MakeLogicalJoin
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

	const XMLCh *join_type_xml = ExtractAttrValue(attrs, EdxltokenJoinType, EdxltokenLogicalJoin);
	EdxlJoinType join_type = ParseJoinType(join_type_xml, CDXLTokens::GetDXLTokenStr(EdxltokenLogicalJoin));

	return GPOS_NEW(memory_pool) CDXLLogicalJoin(memory_pool, join_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToDouble
//
//	@doc:
//	  Converts the attribute value to CDouble
//
//---------------------------------------------------------------------------
CDouble
CDXLOperatorFactory::ConvertAttrValueToDouble
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
//		CDXLOperatorFactory::ExtractConvertAttrValueToDouble
//
//	@doc:
//	  Extracts the value for the given attribute and converts it into CDouble
//
//---------------------------------------------------------------------------
CDouble
CDXLOperatorFactory::ExtractConvertAttrValueToDouble
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
	return ConvertAttrValueToDouble
			(
			memory_manager_dxl,
			xmlszValue,
			edxltokenAttr,
			edxltokenElement
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ConvertAttrValueToLint
//
//	@doc:
//	  Converts the attribute value to LINT
//
//---------------------------------------------------------------------------
LINT
CDXLOperatorFactory::ConvertAttrValueToLint
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
//		CDXLOperatorFactory::ExtractConvertAttrValueToLint
//
//	@doc:
//	  Extracts the value for the given attribute and converts it into LINT
//
//---------------------------------------------------------------------------
LINT
CDXLOperatorFactory::ExtractConvertAttrValueToLint
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

	return ConvertAttrValueToLint(memory_manager_dxl, xmlszValue, edxltokenAttr, edxltokenElement);
}


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
	ULONG type = CDXLOperatorFactory::ConvertAttrValueToUlong(memory_manager_dxl, xmlszType, edxltokenAttr, edxltokenElement);
	
	XMLCh *xml_str_name = xmlsztokSysid.nextToken();
	CWStringDynamic *str_name = CDXLUtils::CreateDynamicStringFromXMLChArray(memory_manager_dxl, xml_str_name);
	
	CSystemId sysid((IMDId::EMDIdType) type, str_name->GetBuffer(), str_name->Length());	
	GPOS_DELETE(str_name);
	
	return sysid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::MakeWindowRef
//
//	@doc:
//		Construct an WindowRef operator
//
//---------------------------------------------------------------------------
CDXLScalar*
CDXLOperatorFactory::MakeWindowRef
	(
	CDXLMemoryManager *memory_manager_dxl,
	const Attributes &attrs
	)
{
	// get the memory pool from the memory manager
	IMemoryPool *memory_pool = memory_manager_dxl->Pmp();
	IMDId *mdid_func = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenWindowrefOid, EdxltokenScalarWindowref);
	IMDId *mdid_return_type = ExtractConvertAttrValueToMdId(memory_manager_dxl, attrs, EdxltokenTypeId, EdxltokenScalarWindowref);
	BOOL fDistinct = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenWindowrefDistinct, EdxltokenScalarWindowref);
	BOOL is_star_arg = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenWindowrefStarArg, EdxltokenScalarWindowref);
	BOOL is_simple_agg = ExtractConvertAttrValueToBool(memory_manager_dxl, attrs, EdxltokenWindowrefSimpleAgg, EdxltokenScalarWindowref);
	ULONG win_spec_pos = ExtractConvertAttrValueToUlong(memory_manager_dxl, attrs, EdxltokenWindowrefWinSpecPos, EdxltokenScalarWindowref);

	const XMLCh *xmlszStage  = ExtractAttrValue(attrs, EdxltokenWindowrefStrategy, EdxltokenScalarWindowref);
	EdxlWinStage dxl_win_stage = EdxlwinstageSentinel;

	ULONG rgrgulMapping[][2] =
					{
					{EdxlwinstageImmediate, EdxltokenWindowrefStageImmediate},
					{EdxlwinstagePreliminary, EdxltokenWindowrefStagePreliminary},
					{EdxlwinstageRowKey, EdxltokenWindowrefStageRowKey}
					};

	const ULONG arity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		Edxltoken edxltk = (Edxltoken) pulElem[1];
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(edxltk), xmlszStage))
		{
			dxl_win_stage = (EdxlWinStage) pulElem[0];
			break;
		}
	}
	GPOS_ASSERT(EdxlwinstageSentinel != dxl_win_stage);

	return GPOS_NEW(memory_pool) CDXLScalarWindowRef(memory_pool, mdid_func, mdid_return_type, fDistinct, is_star_arg, is_simple_agg, dxl_win_stage, win_spec_pos);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseCmpType
//
//	@doc:
//		Parse comparison type
//
//---------------------------------------------------------------------------
IMDType::ECmpType
CDXLOperatorFactory::ParseCmpType
	(
	const XMLCh* xml_str_comp_type
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
				xml_str_comp_type))
		{
			return (IMDType::ECmpType) pulElem[1];
		}
	}
	
	GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLInvalidAttributeValue, CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOpCmpType)->GetBuffer(), CDXLTokens::GetDXLTokenStr(EdxltokenGPDBScalarOp)->GetBuffer());		
	return (IMDType::ECmpType) IMDType::EcmptOther;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperatorFactory::ParseRelationDistPolicy
//
//	@doc:
//		Parse relation distribution policy from XML string
//
//---------------------------------------------------------------------------
IMDRelation::GetRelDistrPolicy
CDXLOperatorFactory::ParseRelationDistPolicy
	(
	const XMLCh* xmlsz
	)
{
	GPOS_ASSERT(NULL != xmlsz);
	IMDRelation::GetRelDistrPolicy rel_distr_policy = IMDRelation::EreldistrSentinel;
	
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
//		CDXLOperatorFactory::ParseRelationStorageType
//
//	@doc:
//		Parse relation storage type from XML string
//
//---------------------------------------------------------------------------
IMDRelation::Erelstoragetype
CDXLOperatorFactory::ParseRelationStorageType
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
//		CDXLOperatorFactory::ParseOnCommitActionSpec
//
//	@doc:
//		Parse on commit action spec from XML attributes
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions::ECtasOnCommitAction
CDXLOperatorFactory::ParseOnCommitActionSpec
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
//		CDXLOperatorFactory::ParseIndexType
//
//	@doc:
//		Parse index type from XML attributes
//
//---------------------------------------------------------------------------
IMDIndex::EmdindexType
CDXLOperatorFactory::ParseIndexType
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
