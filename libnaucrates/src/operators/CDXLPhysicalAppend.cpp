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
//		CDXLPhysicalAppend::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalAppend::Edxlop() const
{
	return EdxlopPhysicalAppend;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAppend::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalAppend::PstrOpName() const
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
	const CWStringConst *pstrElemName = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAppendIsTarget), m_fIsTarget);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAppendIsZapped), m_fIsZapped);
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);		
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
	BOOL fValidateChildren
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, fValidateChildren);
	
	const ULONG ulChildren = pdxln->UlArity();
	for (ULONG ul = EdxlappendIndexFirstChild; ul < ulChildren; ul++)
	{
		CDXLNode *pdxlnChild = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypePhysical == pdxlnChild->Pdxlop()->Edxloperatortype());
		
		if (fValidateChildren)
		{
			pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
		}
	}
	
}
#endif // GPOS_DEBUG

// EOF
