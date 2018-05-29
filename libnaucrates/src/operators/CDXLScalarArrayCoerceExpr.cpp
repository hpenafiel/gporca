//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Inc.
//
//	@filename:
//		CDXLScalarArrayCoerceExpr.cpp
//
//	@doc:
//		Implementation of DXL scalar array coerce expr
//
//	@owner:
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarArrayCoerceExpr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayCoerceExpr::CDXLScalarArrayCoerceExpr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarArrayCoerceExpr::CDXLScalarArrayCoerceExpr
	(
	IMemoryPool *pmp,
	IMDId *pmdidElementFunc,
	IMDId *pmdidResultType,
	INT iTypeModifier,
	BOOL fIsExplicit,
	EdxlCoercionForm edxlcf,
	INT iLoc
	)
	:
	CDXLScalarCoerceBase(pmp, pmdidResultType, iTypeModifier, edxlcf, iLoc),
	m_pmdidElementFunc(pmdidElementFunc),
	m_fIsExplicit(fIsExplicit)
{
	GPOS_ASSERT(NULL != pmdidElementFunc);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayCoerceExpr::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayCoerceExpr::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarArrayCoerceExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayCoerceExpr::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayCoerceExpr::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);

	m_pmdidElementFunc->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenElementFunc));
	PmdidResultType()->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsExplicit), m_fIsExplicit);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCoercionForm), (ULONG) Edxlcf());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenLocation), ILoc());

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
}

// EOF
