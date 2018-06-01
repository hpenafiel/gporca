//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarJoinFilter.cpp
//
//	@doc:
//		Implementation of DXL join filter operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarJoinFilter.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarJoinFilter::CDXLScalarJoinFilter
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarJoinFilter::CDXLScalarJoinFilter
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalarFilter(memory_pool)
{
}



//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarJoinFilter::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarJoinFilter::Edxlop() const
{
	return EdxlopScalarJoinFilter;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarJoinFilter::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarJoinFilter::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarJoinFilter);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarJoinFilter::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarJoinFilter::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	// serilize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);	
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarJoinFilter::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarJoinFilter::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(1 >= pdxln->Arity());
	
	if (1 == pdxln->Arity())
	{
		CDXLNode *pdxlnCond = (*pdxln)[0];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnCond->GetOperator()->Edxloperatortype());
		
		if (validate_children)
		{
			pdxlnCond->GetOperator()->AssertValid(pdxlnCond, validate_children);
		}
	}
	
}
#endif // GPOS_DEBUG


// EOF
