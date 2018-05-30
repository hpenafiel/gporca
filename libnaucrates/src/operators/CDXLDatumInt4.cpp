//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLDatumInt4.cpp
//
//	@doc:
//		Implementation of DXL datum of type integer
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatumInt4.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumInt4::CDXLDatumInt4
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatumInt4::CDXLDatumInt4
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	BOOL is_null,
	INT iVal
	)
	:
	CDXLDatum(memory_pool, mdid_type, IDefaultTypeModifier, is_null, 4 /*length*/ ),
	m_iVal(iVal)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumInt4::IValue
//
//	@doc:
//		Return the integer value
//
//---------------------------------------------------------------------------
INT
CDXLDatumInt4::IValue() const
{
	return m_iVal;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumInt4::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatumInt4::Serialize
	(
	CXMLSerializer *xml_serializer
	)
{
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_is_null);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), IsPassedByValue());
	
	if (!m_is_null)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_iVal);
	}
}

// EOF
