//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalDelete.cpp
//
//	@doc:
//		Implementation of DXL logical delete operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalDelete.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::CDXLLogicalDelete
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalDelete::CDXLLogicalDelete
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc,
	ULONG ulCtid,
	ULONG ulSegmentId,
	ULongPtrArray *pdrgpulDelete
	)
	:
	CDXLLogical(memory_pool),
	m_table_descr_dxl(pdxltabdesc),
	m_ctid_colid(ulCtid),
	m_segid_colid(ulSegmentId),
	m_deletion_colid_array(pdrgpulDelete)
{
	GPOS_ASSERT(NULL != pdxltabdesc);
	GPOS_ASSERT(NULL != pdrgpulDelete);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::~CDXLLogicalDelete
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalDelete::~CDXLLogicalDelete()
{
	m_table_descr_dxl->Release();
	m_deletion_colid_array->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalDelete::GetDXLOperator() const
{
	return EdxlopLogicalDelete;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalDelete::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalDelete);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalDelete::SerializeToDXL
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

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ctid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_segid_colid);

	m_table_descr_dxl->SerializeToDXL(xml_serializer);
	node->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalDelete::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalDelete::AssertValid
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
