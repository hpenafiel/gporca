//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalMergeJoin.cpp
//
//	@doc:
//		Implementation of DXL physical merge join operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalMergeJoin.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMergeJoin::CDXLPhysicalMergeJoin
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalMergeJoin::CDXLPhysicalMergeJoin
	(
	IMemoryPool *memory_pool,
	EdxlJoinType edxljt,
	BOOL fUniqueOuter
	)
	:
	CDXLPhysicalJoin(memory_pool, edxljt),
	m_fUniqueOuter(fUniqueOuter)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMergeJoin::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalMergeJoin::GetDXLOperator() const
{
	return EdxlopPhysicalMergeJoin;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMergeJoin::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalMergeJoin::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalMergeJoin);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMergeJoin::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMergeJoin::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinType), PstrJoinTypeName());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenMergeJoinUniqueOuter), m_fUniqueOuter);

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMergeJoin::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMergeJoin::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT(EdxlmjIndexSentinel == pdxln->Arity());
	GPOS_ASSERT(EdxljtSentinel > Edxltype());
	
	CDXLNode *pdxlnJoinFilter = (*pdxln)[EdxlmjIndexJoinFilter];
	CDXLNode *pdxlnMergeClauses = (*pdxln)[EdxlmjIndexMergeCondList];
	CDXLNode *pdxlnLeft = (*pdxln)[EdxlmjIndexLeftChild];
	CDXLNode *pdxlnRight = (*pdxln)[EdxlmjIndexRightChild];

	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxlopScalarJoinFilter == pdxlnJoinFilter->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxlopScalarMergeCondList == pdxlnMergeClauses->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnLeft->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnRight->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		pdxlnJoinFilter->GetOperator()->AssertValid(pdxlnJoinFilter, validate_children);
		pdxlnMergeClauses->GetOperator()->AssertValid(pdxlnMergeClauses, validate_children);
		pdxlnLeft->GetOperator()->AssertValid(pdxlnLeft, validate_children);
		pdxlnRight->GetOperator()->AssertValid(pdxlnRight, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
