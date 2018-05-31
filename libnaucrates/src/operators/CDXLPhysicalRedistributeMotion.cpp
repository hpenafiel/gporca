//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalRedistributeMotion.cpp
//
//	@doc:
//		Implementation of DXL physical redistribute motion operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalRedistributeMotion.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRedistributeMotion::CDXLPhysicalRedistributeMotion
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalRedistributeMotion::CDXLPhysicalRedistributeMotion
	(
	IMemoryPool *memory_pool,
	BOOL fDuplicateSensitive
	)
	:
	CDXLPhysicalMotion(memory_pool),
	m_fDuplicateSensitive(fDuplicateSensitive)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRedistributeMotion::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalRedistributeMotion::Edxlop() const
{
	return EdxlopPhysicalMotionRedistribute;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRedistributeMotion::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalRedistributeMotion::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalRedistributeMotion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRedistributeMotion::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRedistributeMotion::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	SerializeSegmentInfoToDXL(xml_serializer);
	
	if (m_fDuplicateSensitive)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDuplicateSensitive), true);
	}	
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRedistributeMotion::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRedistributeMotion::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, fValidateChildren);
	
	GPOS_ASSERT(m_pdrgpiInputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiInputSegIds->Size());
	GPOS_ASSERT(m_pdrgpiOutputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiOutputSegIds->Size());
	
	GPOS_ASSERT(EdxlrmIndexSentinel == pdxln->Arity());
	
	CDXLNode *pdxlnChild = (*pdxln)[EdxlrmIndexChild];
	CDXLNode *pdxlnHashExprList = (*pdxln)[EdxlrmIndexHashExprList];

	GPOS_ASSERT(EdxloptypePhysical == pdxlnChild->Pdxlop()->Edxloperatortype());
	
	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
		pdxlnHashExprList->Pdxlop()->AssertValid(pdxlnHashExprList, fValidateChildren);
	}
}
#endif // GPOS_DEBUG

// EOF
