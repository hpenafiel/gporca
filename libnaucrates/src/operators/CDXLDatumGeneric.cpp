//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLDatumGeneric.cpp
//
//	@doc:
//		Implementation of DXL datum of type generic
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatumGeneric.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumGeneric::CDXLDatumGeneric
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatumGeneric::CDXLDatumGeneric
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	INT type_modifier,
	BOOL fByVal,
	BOOL is_null,
	BYTE *pba,
	ULONG length
	)
	:
	CDXLDatum(memory_pool, mdid_type, type_modifier, is_null, length),
	m_fByVal(fByVal),
	m_pba(pba)
{
	GPOS_ASSERT_IMP(m_is_null, (m_pba == NULL) && (m_length == 0));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumGeneric::~CDXLDatumGeneric
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLDatumGeneric::~CDXLDatumGeneric()
{
	GPOS_DELETE_ARRAY(m_pba);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumGeneric::Pba
//
//	@doc:
//		Returns the bytearray of the datum
//
//---------------------------------------------------------------------------
const BYTE *
CDXLDatumGeneric::Pba() const
{
	return m_pba;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatumGeneric::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatumGeneric::Serialize
	(
	CXMLSerializer *xml_serializer
	)
{
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));
	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), m_type_modifier);
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsNull), m_is_null);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIsByValue), m_fByVal);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenValue), m_is_null, Pba(), Length());
}

// EOF
