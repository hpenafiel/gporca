//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLDatumStatsLintMappable.cpp
//
//	@doc:
//		Implementation of DXL datum of types having LINT mapping
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatumStatsLintMappable.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumStatsLintMappable::CDXLDatumStatsLintMappable
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatumStatsLintMappable::CDXLDatumStatsLintMappable
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	BOOL is_passed_by_value,
	BOOL is_null,
	BYTE *byte_array,
	ULONG length,
	LINT lValue
	)
	:
	CDXLDatumGeneric(memory_pool, mdid_type, type_modifier, is_passed_by_value, is_null, byte_array, length),
	m_val(lValue)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumStatsLintMappable::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatumStatsLintMappable::Serialize
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
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), m_is_passed_by_value);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_is_null, GetByteArray(), Length());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenLintValue), GetLINTMapping());
}


// EOF
