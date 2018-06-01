//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalResult.cpp
//
//	@doc:
//		Implementation of DXL physical result operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalResult.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalResult::CDXLPhysicalResult
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalResult::CDXLPhysicalResult
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalResult::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalResult::GetDXLOperator() const
{
	return EdxlopPhysicalResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalResult::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalResult::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalResult);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalResult::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalResult::SerializeToDXL
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

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalResult::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalResult::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{

	GPOS_ASSERT(EdxlresultIndexSentinel >= pdxln->Arity());
	
	// check that one time filter is valid
	CDXLNode *pdxlnOneTimeFilter = (*pdxln)[EdxlresultIndexOneTimeFilter];
	GPOS_ASSERT(EdxlopScalarOneTimeFilter == pdxlnOneTimeFilter->GetOperator()->GetDXLOperator());
	
	if (validate_children)
	{
		pdxlnOneTimeFilter->GetOperator()->AssertValid(pdxlnOneTimeFilter, validate_children);
	}
	
	if (EdxlresultIndexSentinel == pdxln->Arity())
	{
		CDXLNode *child_dxlnode = (*pdxln)[EdxlresultIndexChild];
		GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}

}
#endif // GPOS_DEBUG

// EOF
