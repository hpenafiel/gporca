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
CDXLScalarPartListNullTest::Edxlop() const
{
	return EdxlopScalarPartListNullTest;
}

// Operator name
const CWStringConst *
CDXLScalarPartListNullTest::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarPartListNullTest);
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
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartLevel), m_ulLevel);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenScalarIsNull), m_fIsNull);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
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
	CMDAccessor * //pmda
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
CDXLScalarPartListNullTest::PdxlopConvert
	(
	CDXLOperator *dxl_op
	)
{
	GPOS_ASSERT(NULL != dxl_op);
	GPOS_ASSERT(EdxlopScalarPartListNullTest == dxl_op->Edxlop());

	return dynamic_cast<CDXLScalarPartListNullTest*>(dxl_op);
}

// EOF
