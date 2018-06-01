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
	BOOL validate_children
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);

	GPOS_ASSERT(m_pdrgpiInputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiInputSegIds->Size());
	GPOS_ASSERT(m_pdrgpiOutputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiOutputSegIds->Size());

	GPOS_ASSERT(EdxlrandommIndexSentinel == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlrandommIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->Edxloperatortype());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
