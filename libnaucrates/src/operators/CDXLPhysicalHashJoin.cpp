//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalHashJoin.cpp
//
//	@doc:
//		Implementation of DXL physical hash join operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalHashJoin.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalHashJoin::CDXLPhysicalHashJoin
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalHashJoin::CDXLPhysicalHashJoin
	(
	IMemoryPool *memory_pool,
	EdxlJoinType edxljt
	)
	:
	CDXLPhysicalJoin(memory_pool, edxljt)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalHashJoin::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalHashJoin::GetDXLOperator() const
{
	return EdxlopPhysicalHashJoin;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalHashJoin::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalHashJoin::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalHashJoin);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalHashJoin::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalHashJoin::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinType), GetJoinTypeNameStr());
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalHashJoin::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalHashJoin::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT(EdxlhjIndexSentinel == pdxln->Arity());
	GPOS_ASSERT(EdxljtSentinel > GetJoinType());
	
	CDXLNode *pdxlnJoinFilter = (*pdxln)[EdxlhjIndexJoinFilter];
	CDXLNode *pdxlnHashClauses = (*pdxln)[EdxlhjIndexHashCondList];
	CDXLNode *pdxlnLeft = (*pdxln)[EdxlhjIndexHashLeft];
	CDXLNode *pdxlnRight = (*pdxln)[EdxlhjIndexHashRight];

	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxlopScalarJoinFilter == pdxlnJoinFilter->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxlopScalarHashCondList == pdxlnHashClauses->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnLeft->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnRight->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		pdxlnJoinFilter->GetOperator()->AssertValid(pdxlnJoinFilter, validate_children);
		pdxlnHashClauses->GetOperator()->AssertValid(pdxlnHashClauses, validate_children);
		pdxlnLeft->GetOperator()->AssertValid(pdxlnLeft, validate_children);
		pdxlnRight->GetOperator()->AssertValid(pdxlnRight, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
