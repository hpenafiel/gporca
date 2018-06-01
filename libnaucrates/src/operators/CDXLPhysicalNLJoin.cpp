//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalNLJoin.cpp
//
//	@doc:
//		Implementation of DXL physical nested loop join operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalNLJoin.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::CDXLPhysicalNLJoin
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalNLJoin::CDXLPhysicalNLJoin
	(
	IMemoryPool *memory_pool,
	EdxlJoinType edxljt,
	BOOL fIndexNLJ
	)
	:
	CDXLPhysicalJoin(memory_pool, edxljt),
	m_fIndexNLJ(fIndexNLJ)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalNLJoin::GetDXLOperator() const
{
	return EdxlopPhysicalNLJoin;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalNLJoin::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalNLJoin);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalNLJoin::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinType), PstrJoinTypeName());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPhysicalNLJoinIndex), m_fIndexNLJ);


	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalNLJoin::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT(EdxlnljIndexSentinel == pdxln->Arity());
	GPOS_ASSERT(EdxljtSentinel > Edxltype());
	
	CDXLNode *pdxlnJoinFilter = (*pdxln)[EdxlnljIndexJoinFilter];
	CDXLNode *pdxlnLeft = (*pdxln)[EdxlnljIndexLeftChild];
	CDXLNode *pdxlnRight = (*pdxln)[EdxlnljIndexRightChild];
	
	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxlopScalarJoinFilter == pdxlnJoinFilter->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnLeft->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnRight->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		pdxlnJoinFilter->GetOperator()->AssertValid(pdxlnJoinFilter, validate_children);
		pdxlnLeft->GetOperator()->AssertValid(pdxlnLeft, validate_children);
		pdxlnRight->GetOperator()->AssertValid(pdxlnRight, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
