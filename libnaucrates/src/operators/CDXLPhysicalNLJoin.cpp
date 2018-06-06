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
	EdxlJoinType join_type,
	BOOL fIndexNLJ
	)
	:
	CDXLPhysicalJoin(memory_pool, join_type),
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
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalNLJoin);
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
	const CDXLNode *dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);		
	
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenJoinType), GetJoinTypeNameStr());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalNLJoinIndex), m_fIndexNLJ);


	// serialize properties
	dxlnode->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	dxlnode->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);		
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
	const CDXLNode *dxlnode,
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(dxlnode, validate_children);
	
	GPOS_ASSERT(EdxlnljIndexSentinel == dxlnode->Arity());
	GPOS_ASSERT(EdxljtSentinel > GetJoinType());
	
	CDXLNode *pdxlnJoinFilter = (*dxlnode)[EdxlnljIndexJoinFilter];
	CDXLNode *pdxlnLeft = (*dxlnode)[EdxlnljIndexLeftChild];
	CDXLNode *pdxlnRight = (*dxlnode)[EdxlnljIndexRightChild];
	
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
