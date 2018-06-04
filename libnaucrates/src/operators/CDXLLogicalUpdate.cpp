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
	CDXLTableDescr *pdxltabdesc,
	ULONG ulCtid,
	ULONG ulSegmentId,
	ULongPtrArray *pdrgpulDelete,
	ULongPtrArray *pdrgpulInsert,
	BOOL fPreserveOids,
	ULONG ulTupleOid
	)
	:
	CDXLLogical(memory_pool),
	m_table_descr_dxl(pdxltabdesc),
	m_ctid_colid(ulCtid),
	m_segid_colid(ulSegmentId),
	m_deletion_colid_array(pdrgpulDelete),
	m_pdrgpulInsert(pdrgpulInsert),
	m_fPreserveOids(fPreserveOids),
	m_ulTupleOid(ulTupleOid)
{
	GPOS_ASSERT(NULL != pdxltabdesc);
	GPOS_ASSERT(NULL != pdrgpulDelete);
	GPOS_ASSERT(NULL != pdrgpulInsert);
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
	m_pdrgpulInsert->Release();
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
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	CWStringDynamic *pstrColsDel = CDXLUtils::Serialize(m_memory_pool, m_deletion_colid_array);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDeleteCols), pstrColsDel);
	GPOS_DELETE(pstrColsDel);

	CWStringDynamic *pstrColsIns = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulInsert);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), pstrColsIns);
	GPOS_DELETE(pstrColsIns);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ctid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_segid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenUpdatePreservesOids), m_fPreserveOids);
	
	if (m_fPreserveOids)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTupleOidColId), m_ulTupleOid);
	}
	
	m_table_descr_dxl->SerializeToDXL(xml_serializer);
	pdxln->SerializeChildrenToDXL(xml_serializer);

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
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());

	CDXLNode *child_dxlnode = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
