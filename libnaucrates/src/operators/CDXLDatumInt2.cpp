//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLDatumInt2.cpp
//
//	@doc:
//		Implementation of DXL datum of type short integer
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatumInt2.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumInt2::CDXLDatumInt2
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatumInt2::CDXLDatumInt2
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	BOOL is_null,
	SINT sVal
	)
	:
	CDXLDatum(memory_pool, mdid_type, IDefaultTypeModifier, is_null, 2 /*length*/ ),
	m_sVal(sVal)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumInt2::SValue
//
//	@doc:
//		Return the short integer value
//
//---------------------------------------------------------------------------
SINT
CDXLDatumInt2::SValue() const
{
	return m_sVal;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumInt2::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatumInt2::Serialize
	(
	CXMLSerializer *xml_serializer
	)
{
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_is_null);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), IsPassedByValue());
	
	if (!m_is_null)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_sVal);
	}
}

// EOF
