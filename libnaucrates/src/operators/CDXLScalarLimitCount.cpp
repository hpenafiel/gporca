//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarLimitCount.cpp
//
//	@doc:
//		Implementation of DXL Scalar Limit Count
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarLimitCount.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarLimitCount::CDXLScalarLimitCount
//
//	@doc:
//		Constructs a scalar Limit Count node
//
//---------------------------------------------------------------------------
CDXLScalarLimitCount::CDXLScalarLimitCount
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalar(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarLimitCount::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarLimitCount::GetDXLOperator() const
{
	return EdxlopScalarLimitCount;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarLimitCount::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarLimitCount::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarLimitCount);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarLimitCount::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarLimitCount::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarLimitCount::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarLimitCount::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(1 >= ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
