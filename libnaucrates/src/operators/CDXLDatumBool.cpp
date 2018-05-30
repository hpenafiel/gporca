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
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	BOOL is_null,
	BOOL value
	)
	:
	CDXLDatum(memory_pool, mdid_type, IDefaultTypeModifier, is_null, 1 /*length*/),
	m_value(value)
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
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_is_null);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), IsPassedByValue());

	if (!m_is_null)
	{
		if(m_value)
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
