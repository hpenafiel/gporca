//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalAppend.cpp
//
//	@doc:
//		Implementation of DXL physical Append operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalAppend.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::CDXLPhysicalAppend
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalAppend::CDXLPhysicalAppend
	(
	IMemoryPool *memory_pool,
	BOOL fIsTarget,
	BOOL fIsZapped
	)
	:
	CDXLPhysical(memory_pool),
	m_fIsTarget(fIsTarget),
	m_fIsZapped(fIsZapped)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalAppend::GetDXLOperator() const
{
	return EdxlopPhysicalAppend;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalAppend::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalAppend);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::FIsTarget
//
//	@doc:
//		Is the append node updating a target relation
//
//---------------------------------------------------------------------------
BOOL
CDXLPhysicalAppend::FIsTarget() const
{
	return m_fIsTarget;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::FIsZapped
//
//	@doc:
//		Is the append node zapped
//
//---------------------------------------------------------------------------
BOOL
CDXLPhysicalAppend::FIsZapped() const
{
	return m_fIsZapped;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAppend::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAppendIsTarget), m_fIsTarget);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAppendIsZapped), m_fIsZapped);
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAppend::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	const ULONG ulChildren = pdxln->Arity();
	for (ULONG ul = EdxlappendIndexFirstChild; ul < ulChildren; ul++)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
	
}
#endif // GPOS_DEBUG

// EOF
