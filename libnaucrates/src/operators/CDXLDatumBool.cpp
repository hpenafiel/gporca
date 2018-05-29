//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLDatumBool.cpp
//
//	@doc:
//		Implementation of DXL datum of type boolean
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumBool::CDXLDatumBool
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatumBool::CDXLDatumBool
	(
	IMemoryPool *pmp,
	IMDId *pmdidType,
	BOOL fNull,
	BOOL fVal
	)
	:
	CDXLDatum(pmp, pmdidType, IDefaultTypeModifier, fNull, 1 /*ulLength*/),
	m_fVal(fVal)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumBool::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatumBool::Serialize
	(
	CXMLSerializer *xml_serializer
	)
{
	m_pmdidType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_fNull);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), FByValue());

	if (!m_fNull)
	{
		if(m_fVal)
		{
			xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), CDXLTokens::PstrToken(EdxltokenTrue));
		}
		else
		{
			xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), CDXLTokens::PstrToken(EdxltokenFalse));
		}
	}
}

// EOF
