//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalGatherMotion.cpp
//
//	@doc:
//		Implementation of DXL physical gather motion operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalGatherMotion.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalGatherMotion::CDXLPhysicalGatherMotion
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalGatherMotion::CDXLPhysicalGatherMotion
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysicalMotion(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalGatherMotion::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalGatherMotion::GetDXLOperator() const
{
	return EdxlopPhysicalMotionGather;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalGatherMotion::IOutputSegIdx
//
//	@doc:
//		Output segment index
//
//---------------------------------------------------------------------------
INT
CDXLPhysicalGatherMotion::IOutputSegIdx() const
{
	GPOS_ASSERT(NULL != m_pdrgpiOutputSegIds);
	GPOS_ASSERT(1 == m_pdrgpiOutputSegIds->Size());
	return *((*m_pdrgpiOutputSegIds)[0]);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalGatherMotion::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalGatherMotion::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalGatherMotion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalGatherMotion::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalGatherMotion::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
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
//		CDXLPhysicalGatherMotion::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalGatherMotion::AssertValid
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
	GPOS_ASSERT(1 == m_pdrgpiOutputSegIds->Size());

	GPOS_ASSERT(EdxlgmIndexSentinel == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlgmIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
