//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarNullTest.cpp
//
//	@doc:
//		Implementation of DXL NullTest
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarNullTest.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullTest::CDXLScalarNullTest
//
//	@doc:
//		Constructs a NullTest node
//
//---------------------------------------------------------------------------
CDXLScalarNullTest::CDXLScalarNullTest
	(
	IMemoryPool *memory_pool,
	BOOL fIsNull
	)
	:
	CDXLScalar(memory_pool),
	m_fIsNull(fIsNull)
{

}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullTest::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarNullTest::GetDXLOperator() const
{
	return EdxlopScalarNullTest;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullTest::FIsNullTest
//
//	@doc:
//		Null Test type (is null or is not null)
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarNullTest::FIsNullTest() const
{
	return m_fIsNull;
}



//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullTest::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarNullTest::GetOpNameStr() const
{
	if(m_fIsNull)
	{
		return CDXLTokens::GetDXLTokenStr(EdxltokenScalarIsNull);
	}
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarIsNotNull);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullTest::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarNullTest::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullTest::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarNullTest::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(1 == pdxln->Arity());

	CDXLNode *pdxlnArg = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
