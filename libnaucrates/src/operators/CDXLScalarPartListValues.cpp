//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal, Inc.
//
//	Implementation of DXL Part List Values expression
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarPartListValues.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

// Ctor
CDXLScalarPartListValues::CDXLScalarPartListValues
	(
	IMemoryPool *memory_pool,
	ULONG ulLevel,
	IMDId *pmdidResult,
	IMDId *pmdidElement
	)
	:
	CDXLScalar(memory_pool),
	m_ulLevel(ulLevel),
	m_pmdidResult(pmdidResult),
	m_pmdidElement(pmdidElement)
{
	GPOS_ASSERT(pmdidResult->IsValid());
	GPOS_ASSERT(pmdidElement->IsValid());
}


// Dtor
CDXLScalarPartListValues::~CDXLScalarPartListValues()
{
	m_pmdidResult->Release();
	m_pmdidElement->Release();
}

// Operator type
Edxlopid
CDXLScalarPartListValues::GetDXLOperator() const
{
	return EdxlopScalarPartListValues;
}

// Operator name
const CWStringConst *
CDXLScalarPartListValues::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarPartListValues);
}

// partitioning level
ULONG
CDXLScalarPartListValues::UlLevel() const
{
	return m_ulLevel;
}

// result type
IMDId *
CDXLScalarPartListValues::PmdidResult() const
{
	return m_pmdidResult;
}

// element type
IMDId *
CDXLScalarPartListValues::PmdidElement() const
{
	return m_pmdidElement;
}

// does the operator return a boolean result
BOOL
CDXLScalarPartListValues::FBoolean
	(
	CMDAccessor * //pmda
	)
	const
{
	return false;
}

// Serialize operator in DXL format
void
CDXLScalarPartListValues::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * // pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartLevel), m_ulLevel);
	m_pmdidResult->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenGPDBScalarOpResultTypeId));
	m_pmdidElement->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenArrayElementType));
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
// Checks whether operator node is well-structured
void
CDXLScalarPartListValues::AssertValid
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
CDXLScalarPartListValues *
CDXLScalarPartListValues::Cast
	(
	CDXLOperator *dxl_op
	)
{
	GPOS_ASSERT(NULL != dxl_op);
	GPOS_ASSERT(EdxlopScalarPartListValues == dxl_op->GetDXLOperator());

	return dynamic_cast<CDXLScalarPartListValues*>(dxl_op);
}

// EOF
