//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarBooleanTest.cpp
//
//	@doc:
//		Implementation of DXL BooleanTest
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarBooleanTest.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBooleanTest::CDXLScalarBooleanTest
//
//	@doc:
//		Constructs a BooleanTest node
//
//---------------------------------------------------------------------------
CDXLScalarBooleanTest::CDXLScalarBooleanTest
	(
	IMemoryPool *memory_pool,
	const EdxlBooleanTestType edxlbooleanTestType
	)
	:
	CDXLScalar(memory_pool),
	m_edxlbooleantesttype(edxlbooleanTestType)
{

}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBooleanTest::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarBooleanTest::GetDXLOperator() const
{
	return EdxlopScalarBooleanTest;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBooleanTest::EdxlBoolType
//
//	@doc:
//		Boolean Test type
//
//---------------------------------------------------------------------------
EdxlBooleanTestType
CDXLScalarBooleanTest::EdxlBoolType() const
{
	return m_edxlbooleantesttype;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBooleanTest::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarBooleanTest::GetOpNameStr() const
{
	switch (m_edxlbooleantesttype)
	{
		case EdxlbooleantestIsTrue:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolTestIsTrue);
		case EdxlbooleantestIsNotTrue:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolTestIsNotTrue);
		case EdxlbooleantestIsFalse:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolTestIsFalse);
		case EdxlbooleantestIsNotFalse:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolTestIsNotFalse);
		case EdxlbooleantestIsUnknown:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolTestIsUnknown);
		case EdxlbooleantestIsNotUnknown:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarBoolTestIsNotUnknown);
		default:
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBooleanTest::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarBooleanTest::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	GPOS_ASSERT(NULL != element_name);
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBooleanTest::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarBooleanTest::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{

	EdxlBooleanTestType edxlbooltype = ((CDXLScalarBooleanTest *) pdxln->GetOperator())->EdxlBoolType();

	GPOS_ASSERT( (EdxlbooleantestIsTrue == edxlbooltype) || (EdxlbooleantestIsNotTrue == edxlbooltype) || (EdxlbooleantestIsFalse == edxlbooltype)
			|| (EdxlbooleantestIsNotFalse == edxlbooltype)|| (EdxlbooleantestIsUnknown == edxlbooltype)|| (EdxlbooleantestIsNotUnknown == edxlbooltype));

	GPOS_ASSERT(1 == pdxln->Arity());
	CDXLNode *dxlnode_arg = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeScalar == dxlnode_arg->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		dxlnode_arg->GetOperator()->AssertValid(dxlnode_arg, validate_children);
	}

}

#endif // GPOS_DEBUG

// EOF
