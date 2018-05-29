//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLDatumStatsDoubleMappable.cpp
//
//	@doc:
//		Implementation of DXL datum of types having double mapping
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatumStatsDoubleMappable.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumStatsDoubleMappable::CDXLDatumStatsDoubleMappable
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatumStatsDoubleMappable::CDXLDatumStatsDoubleMappable
	(
	IMemoryPool *pmp,
	IMDId *pmdidType,
	INT iTypeModifier,
	BOOL fByVal,
	BOOL fNull,
	BYTE *pba,
	ULONG ulLength,
	CDouble dValue
	)
	:
	CDXLDatumGeneric(pmp, pmdidType, iTypeModifier, fByVal, fNull, pba, ulLength),
	m_dValue(dValue)
{
}
//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumStatsDoubleMappable::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatumStatsDoubleMappable::Serialize
	(
	CXMLSerializer *xml_serializer
	)
{
	m_pmdidType->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_fNull);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), m_fByVal);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_fNull, Pba(), Length());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDoubleValue), DStatsMapping());
}

// EOF
