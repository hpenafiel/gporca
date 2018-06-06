//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal, Inc.
//
//  Implementation of DXL Part list null test expression

#include "naucrates/dxl/operators/CDXLScalarPartListNullTest.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

// Ctor
CDXLScalarPartListNullTest::CDXLScalarPartListNullTest
	(
	IMemoryPool *memory_pool,
	ULONG ulLevel,
	BOOL fIsNull
	)
	:
	CDXLScalar(memory_pool),
	m_ulLevel(ulLevel),
	m_fIsNull(fIsNull)
{
}

// Operator type
Edxlopid
CDXLScalarPartListNullTest::GetDXLOperator() const
{
	return EdxlopScalarPartListNullTest;
}

// Operator name
const CWStringConst *
CDXLScalarPartListNullTest::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarPartListNullTest);
}

// Serialize operator in DXL format
void
CDXLScalarPartListNullTest::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * // pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPartLevel), m_ulLevel);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenScalarIsNull), m_fIsNull);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

// partitioning level
ULONG
CDXLScalarPartListNullTest::UlLevel() const
{
	return m_ulLevel;
}

// Null Test type (true for 'is null', false for 'is not null')
BOOL
CDXLScalarPartListNullTest::FIsNull() const
{
	return m_fIsNull;
}

// does the operator return a boolean result
BOOL
CDXLScalarPartListNullTest::FBoolean
	(
	CMDAccessor * //md_accessor
	)
	const
{
	return true;
}

#ifdef GPOS_DEBUG
// Checks whether operator node is well-structured
void
CDXLScalarPartListNullTest::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL // validate_children
	)
	const
{
	GPOS_ASSERT(0 == pdxln->Arity());
}
#endif // GPOS_DEBUG

// conversion function
CDXLScalarPartListNullTest *
CDXLScalarPartListNullTest::Cast
	(
	CDXLOperator *dxl_op
	)
{
	GPOS_ASSERT(NULL != dxl_op);
	GPOS_ASSERT(EdxlopScalarPartListNullTest == dxl_op->GetDXLOperator());

	return dynamic_cast<CDXLScalarPartListNullTest*>(dxl_op);
}

// EOF
