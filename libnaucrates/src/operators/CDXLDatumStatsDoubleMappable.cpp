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
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	BOOL fByVal,
	BOOL is_null,
	BYTE *pba,
	ULONG length,
	CDouble dValue
	)
	:
	CDXLDatumGeneric(memory_pool, mdid_type, type_modifier, fByVal, is_null, pba, length),
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
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_is_null);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), m_fByVal);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_is_null, Pba(), Length());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDoubleValue), DStatsMapping());
}

// EOF
