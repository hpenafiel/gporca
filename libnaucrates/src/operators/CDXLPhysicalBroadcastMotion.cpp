//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalBroadcastMotion.cpp
//
//	@doc:
//		Implementation of DXL physical broadcast motion operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalBroadcastMotion.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalBroadcastMotion::CDXLPhysicalBroadcastMotion
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalBroadcastMotion::CDXLPhysicalBroadcastMotion
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysicalMotion(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalBroadcastMotion::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalBroadcastMotion::Edxlop() const
{
	return EdxlopPhysicalMotionBroadcast;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalBroadcastMotion::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalBroadcastMotion::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalBroadcastMotion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalBroadcastMotion::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalBroadcastMotion::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	SerializeSegmentInfoToDXL(xml_serializer);

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalBroadcastMotion::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalBroadcastMotion::AssertValid
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

	GPOS_ASSERT(EdxlbmIndexSentinel == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlbmIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->Edxloperatortype());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
