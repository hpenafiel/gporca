//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalRandomMotion.cpp
//
//	@doc:
//		Implementation of DXL physical random motion operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalRandomMotion.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRandomMotion::CDXLPhysicalRandomMotion
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalRandomMotion::CDXLPhysicalRandomMotion
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
//		CDXLPhysicalRandomMotion::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalRandomMotion::Edxlop() const
{
	return EdxlopPhysicalMotionRandom;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRandomMotion::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalRandomMotion::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalRandomMotion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRandomMotion::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRandomMotion::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
	
	SerializeSegmentInfoToDXL(xml_serializer);
	
	if (m_fDuplicateSensitive)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDuplicateSensitive), true);
	}

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRandomMotion::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRandomMotion::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, fValidateChildren);

	GPOS_ASSERT(m_pdrgpiInputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiInputSegIds->Size());
	GPOS_ASSERT(m_pdrgpiOutputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiOutputSegIds->Size());

	GPOS_ASSERT(EdxlrandommIndexSentinel == pdxln->Arity());
	
	CDXLNode *pdxlnChild = (*pdxln)[EdxlrandommIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == pdxlnChild->Pdxlop()->Edxloperatortype());
	
	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}
#endif // GPOS_DEBUG

// EOF
