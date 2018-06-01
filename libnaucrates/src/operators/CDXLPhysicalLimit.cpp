//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalLimit.cpp
//
//	@doc:
//		Implementation of DXL physical LIMIT operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalLimit.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalLimit::CDXLPhysicalLimit
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalLimit::CDXLPhysicalLimit
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalLimit::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalLimit::GetDXLOperator() const
{
	return EdxlopPhysicalLimit;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalLimit::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalLimit::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalLimit);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalLimit::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalLimit::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children nodes

	const DXLNodeArray *dxl_array = pdxln->GetChildDXLNodeArray();

	GPOS_ASSERT(4 == pdxln->Arity());
	// serialize the first two children: target-list and plan
	for (ULONG i = 0; i < 4; i++)
	{
		CDXLNode *child_dxlnode = (*dxl_array)[i];
		child_dxlnode->SerializeToDXL(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}



#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalLimit::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalLimit::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(4 == pdxln->Arity());

	// Assert proj list is valid
	CDXLNode *pdxlnProjList = (*pdxln)[EdxllimitIndexProjList];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnProjList->GetOperator()->GetDXLOperator());

	// assert child plan is a physical plan and is valid

	CDXLNode *child_dxlnode = (*pdxln)[EdxllimitIndexChildPlan];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	// Assert the validity of Count and Offset

	CDXLNode *pdxlnCount = (*pdxln)[EdxllimitIndexLimitCount];
	GPOS_ASSERT(EdxlopScalarLimitCount == pdxlnCount->GetOperator()->GetDXLOperator());

	CDXLNode *pdxlnOffset = (*pdxln)[EdxllimitIndexLimitOffset];
	GPOS_ASSERT(EdxlopScalarLimitOffset == pdxlnOffset->GetOperator()->GetDXLOperator());

	if (validate_children)
	{
		pdxlnProjList->GetOperator()->AssertValid(pdxlnProjList, validate_children);
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		pdxlnCount->GetOperator()->AssertValid(pdxlnCount, validate_children);
		pdxlnOffset->GetOperator()->AssertValid(pdxlnOffset, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
