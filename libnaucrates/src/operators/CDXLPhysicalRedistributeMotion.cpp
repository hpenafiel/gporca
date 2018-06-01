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
//		CDXLPhysicalRedistributeMotion::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalRedistributeMotion::GetDXLOperator() const
{
	return EdxlopPhysicalMotionRedistribute;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRedistributeMotion::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalRedistributeMotion::GetOpNameStr() const
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
	const CWStringConst *element_name = GetOpNameStr();
	
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
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT(m_pdrgpiInputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiInputSegIds->Size());
	GPOS_ASSERT(m_pdrgpiOutputSegIds != NULL);
	GPOS_ASSERT(0 < m_pdrgpiOutputSegIds->Size());
	
	GPOS_ASSERT(EdxlrmIndexSentinel == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlrmIndexChild];
	CDXLNode *pdxlnHashExprList = (*pdxln)[EdxlrmIndexHashExprList];

	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		pdxlnHashExprList->GetOperator()->AssertValid(pdxlnHashExprList, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
