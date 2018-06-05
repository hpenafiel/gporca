//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalRoutedDistributeMotion.cpp
//
//	@doc:
//		Implementation of DXL physical routed redistribute motion operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalRoutedDistributeMotion.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRoutedDistributeMotion::CDXLPhysicalRoutedDistributeMotion
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalRoutedDistributeMotion::CDXLPhysicalRoutedDistributeMotion
	(
	IMemoryPool *memory_pool,
	ULONG ulSegmentIdCol
	)
	:
	CDXLPhysicalMotion(memory_pool),
	m_ulSegmentIdCol(ulSegmentIdCol)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRoutedDistributeMotion::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalRoutedDistributeMotion::GetDXLOperator() const
{
	return EdxlopPhysicalMotionRoutedDistribute;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRoutedDistributeMotion::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalRoutedDistributeMotion::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalRoutedDistributeMotion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRoutedDistributeMotion::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRoutedDistributeMotion::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSegmentIdCol), m_ulSegmentIdCol);
	
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
//		CDXLPhysicalRoutedDistributeMotion::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRoutedDistributeMotion::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT(m_input_segids_array != NULL);
	GPOS_ASSERT(0 < m_input_segids_array->Size());
	GPOS_ASSERT(m_output_segids_array != NULL);
	GPOS_ASSERT(0 < m_output_segids_array->Size());
	
	GPOS_ASSERT(EdxlroutedmIndexSentinel == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlroutedmIndexChild];

	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
