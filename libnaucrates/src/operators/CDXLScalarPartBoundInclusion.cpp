//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarPartBoundInclusion.cpp
//
//	@doc:
//		Implementation of DXL Part bound inclusion expression
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarPartBoundInclusion.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundInclusion::CDXLScalarPartBoundInclusion
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarPartBoundInclusion::CDXLScalarPartBoundInclusion
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
//		CDXLScalarPartBoundInclusion::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarPartBoundInclusion::GetDXLOperator() const
{
	return EdxlopScalarPartBoundInclusion;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundInclusion::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarPartBoundInclusion::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarPartBoundInclusion);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartBoundInclusion::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarPartBoundInclusion::SerializeToDXL
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
//		CDXLScalarPartBoundInclusion::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarPartBoundInclusion::AssertValid
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
