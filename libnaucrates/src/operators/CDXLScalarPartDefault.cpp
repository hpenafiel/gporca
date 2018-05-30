//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarPartDefault.cpp
//
//	@doc:
//		Implementation of DXL Part Default expression
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarPartDefault.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartDefault::CDXLScalarPartDefault
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarPartDefault::CDXLScalarPartDefault
	(
	IMemoryPool *memory_pool,
	ULONG ulLevel
	)
	:
	CDXLScalar(memory_pool),
	m_ulLevel(ulLevel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartDefault::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarPartDefault::Edxlop() const
{
	return EdxlopScalarPartDefault;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartDefault::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarPartDefault::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarPartDefault);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartDefault::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarPartDefault::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * // pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartLevel), m_ulLevel);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarPartDefault::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarPartDefault::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL // fValidateChildren
	)
	const
{
	GPOS_ASSERT(0 == pdxln->UlArity());
}
#endif // GPOS_DEBUG

// EOF
