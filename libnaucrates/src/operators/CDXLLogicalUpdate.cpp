//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalUpdate.cpp
//
//	@doc:
//		Implementation of DXL logical update operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalUpdate.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::CDXLLogicalUpdate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalUpdate::CDXLLogicalUpdate
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *table_descr,
	ULONG ctid_colid,
	ULONG segid_colid,
	ULongPtrArray *delete_colid_array,
	ULongPtrArray *insert_colid_array,
	BOOL preserve_oids,
	ULONG tuple_oid
	)
	:
	CDXLLogical(memory_pool),
	m_table_descr_dxl(table_descr),
	m_ctid_colid(ctid_colid),
	m_segid_colid(segid_colid),
	m_deletion_colid_array(delete_colid_array),
	m_insert_colid_array(insert_colid_array),
	m_preserve_oids(preserve_oids),
	m_tuple_oid(tuple_oid)
{
	GPOS_ASSERT(NULL != table_descr);
	GPOS_ASSERT(NULL != delete_colid_array);
	GPOS_ASSERT(NULL != insert_colid_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::~CDXLLogicalUpdate
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalUpdate::~CDXLLogicalUpdate()
{
	m_table_descr_dxl->Release();
	m_deletion_colid_array->Release();
	m_insert_colid_array->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalUpdate::GetDXLOperator() const
{
	return EdxlopLogicalUpdate;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalUpdate::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalUpdate);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalUpdate::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *node
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	CWStringDynamic *deletion_colids = CDXLUtils::Serialize(m_memory_pool, m_deletion_colid_array);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDeleteCols), deletion_colids);
	GPOS_DELETE(deletion_colids);

	CWStringDynamic *insertion_colids = CDXLUtils::Serialize(m_memory_pool, m_insert_colid_array);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), insertion_colids);
	GPOS_DELETE(insertion_colids);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ctid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_segid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenUpdatePreservesOids), m_preserve_oids);
	
	if (m_preserve_oids)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTupleOidColId), m_tuple_oid);
	}
	
	m_table_descr_dxl->SerializeToDXL(xml_serializer);
	node->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalUpdate::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalUpdate::AssertValid
	(
	const CDXLNode *node,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(1 == node->Arity());

	CDXLNode *child_dxlnode = (*node)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
