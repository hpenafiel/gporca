//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalSequence.cpp
//
//	@doc:
//		Implementation of DXL physical sequence operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalSequence.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::CDXLPhysicalSequence
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSequence::CDXLPhysicalSequence
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::~CDXLPhysicalSequence
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSequence::~CDXLPhysicalSequence()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalSequence::GetDXLOperator() const
{
	return EdxlopPhysicalSequence;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalSequence::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenPhysicalSequence);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSequence::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
		
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSequence::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSequence::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{

	const ULONG arity = pdxln->Arity();  
	GPOS_ASSERT(1 < arity);

	for (ULONG ul = 1; ul < arity; ul++)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}

}
#endif // GPOS_DEBUG

// EOF
