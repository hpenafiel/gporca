//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarPartBound.cpp
//
//	@doc:
//		Implementation of DXL Part Bound expression
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarPartBound.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::CDXLScalarPartBound
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarPartBound::CDXLScalarPartBound
	(
	IMemoryPool *memory_pool,
	ULONG ulLevel,
	IMDId *mdid_type,
	BOOL fLower
	)
	:
	CDXLScalar(memory_pool),
	m_ulLevel(ulLevel),
	m_mdid_type(mdid_type),
	m_fLower(fLower)
{
	GPOS_ASSERT(mdid_type->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::~CDXLScalarPartBound
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarPartBound::~CDXLScalarPartBound()
{
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarPartBound::Edxlop() const
{
	return EdxlopScalarPartBound;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarPartBound::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarPartBound);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarPartBound::FBoolean(CMDAccessor *pmda) const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_mdid_type)->Eti());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarPartBound::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * // pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartLevel), m_ulLevel);
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMDType));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenScalarPartBoundLower), m_fLower);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBound::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarPartBound::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL // validate_children
	)
	const
{
	GPOS_ASSERT(0 == pdxln->Arity());
}
#endif // GPOS_DEBUG

// EOF
