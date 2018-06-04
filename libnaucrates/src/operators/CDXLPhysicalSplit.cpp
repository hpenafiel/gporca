//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalSplit.cpp
//
//	@doc:
//		Implementation of DXL physical Split operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalSplit.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSplit::CDXLPhysicalSplit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalSplit::CDXLPhysicalSplit
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpulDelete,
	ULongPtrArray *pdrgpulInsert,
	ULONG ulAction,
	ULONG ulCtid,
	ULONG ulSegmentId,
	BOOL fPreserveOids,
	ULONG ulTupleOid
	)
	:
	CDXLPhysical(memory_pool),
	m_deletion_colid_array(pdrgpulDelete),
	m_pdrgpulInsert(pdrgpulInsert),
	m_ulAction(ulAction),
	m_ctid_colid(ulCtid),
	m_segid_colid(ulSegmentId),
	m_fPreserveOids(fPreserveOids),
	m_ulTupleOid(ulTupleOid)
{
	GPOS_ASSERT(NULL != pdrgpulDelete);
	GPOS_ASSERT(NULL != pdrgpulInsert);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSplit::~CDXLPhysicalSplit
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalSplit::~CDXLPhysicalSplit()
{
	m_deletion_colid_array->Release();
	m_pdrgpulInsert->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSplit::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalSplit::GetDXLOperator() const
{
	return EdxlopPhysicalSplit;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSplit::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalSplit::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalSplit);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSplit::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSplit::SerializeToDXL
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

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenActionColId), m_ulAction);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCtidColId), m_ctid_colid);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenGpSegmentIdColId), m_segid_colid);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenUpdatePreservesOids), m_fPreserveOids);

	if (m_fPreserveOids)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTupleOidColId), m_ulTupleOid);
	}
	
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize project list
	(*pdxln)[0]->SerializeToDXL(xml_serializer);

	// serialize physical child
	(*pdxln)[1]->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSplit::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSplit::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(2 == pdxln->Arity());
	CDXLNode *child_dxlnode = (*pdxln)[1];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
