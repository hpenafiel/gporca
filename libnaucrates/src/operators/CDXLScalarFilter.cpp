//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarFilter.cpp
//
//	@doc:
//		Implementation of DXL physical filter operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarFilter.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFilter::CDXLScalarFilter
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarFilter::CDXLScalarFilter
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}



//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFilter::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarFilter::GetDXLOperator() const
{
	return EdxlopScalarFilter;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFilter::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarFilter::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarFilter);;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFilter::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarFilter::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	// serilize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);	
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarFilter::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarFilter::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children 
	) 
	const
{
	GPOS_ASSERT(1 >= pdxln->Arity());
	
	if (1 == pdxln->Arity())
	{
		CDXLNode *child_dxlnode = (*pdxln)[0];
		
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->GetDXLOperatorType());
	
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}

#endif // GPOS_DEBUG


// EOF
