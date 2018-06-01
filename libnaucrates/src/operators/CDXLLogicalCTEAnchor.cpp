//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalCTEAnchor.cpp
//
//	@doc:
//		Implementation of DXL logical CTE anchors
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalCTEAnchor.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEAnchor::CDXLLogicalCTEAnchor
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalCTEAnchor::CDXLLogicalCTEAnchor
	(
	IMemoryPool *memory_pool,
	ULONG ulId
	)
	:
	CDXLLogical(memory_pool),
	m_ulId(ulId)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEAnchor::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalCTEAnchor::GetDXLOperator() const
{
	return EdxlopLogicalCTEAnchor;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEAnchor::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalCTEAnchor::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalCTEAnchor);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEAnchor::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTEAnchor::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCTEId), UlId());
	
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTEAnchor::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTEAnchor::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
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
