//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarPartBoundOpen.cpp
//
//	@doc:
//		Implementation of DXL Part bound openness expression
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarPartBoundOpen.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundOpen::CDXLScalarPartBoundOpen
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarPartBoundOpen::CDXLScalarPartBoundOpen
	(
	IMemoryPool *memory_pool,
	ULONG ulLevel,
	BOOL fLower
	)
	:
	CDXLScalar(memory_pool),
	m_ulLevel(ulLevel),
	m_fLower(fLower)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundOpen::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarPartBoundOpen::GetDXLOperator() const
{
	return EdxlopScalarPartBoundOpen;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundOpen::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarPartBoundOpen::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarPartBoundOpen);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundOpen::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarPartBoundOpen::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * // pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartLevel), m_ulLevel);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenScalarPartBoundLower), m_fLower);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundOpen::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarPartBoundOpen::AssertValid
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
