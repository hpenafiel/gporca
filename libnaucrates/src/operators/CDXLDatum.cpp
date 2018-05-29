//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLDatum.cpp
//
//	@doc:
//		Implementation of DXL datum with type information
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDatum.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatum::CDXLDatum
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDatum::CDXLDatum
	(
	IMemoryPool *pmp,
	IMDId *pmdidType,
	INT iTypeModifier,
	BOOL fNull,
	ULONG ulLength
	)
	:
	m_pmp(pmp),
	m_pmdidType(pmdidType),
	m_iTypeModifier(iTypeModifier),
	m_fNull(fNull),
	m_ulLength(ulLength)
{
	GPOS_ASSERT(m_pmdidType->IsValid());
}

INT
CDXLDatum::ITypeModifier() const
{
	return m_iTypeModifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatum::FNull
//
//	@doc:
//		Is the datum NULL
//
//---------------------------------------------------------------------------
BOOL
CDXLDatum::FNull() const
{
	return m_fNull;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatum::Length
//
//	@doc:
//		Returns the size of the byte array
//
//---------------------------------------------------------------------------
ULONG 
CDXLDatum::Length() const
{
	return m_ulLength;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDatum::Serialize
//
//	@doc:
//		Serialize datum in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDatum::Serialize
	(
	CXMLSerializer *xml_serializer,
	const CWStringConst *pstrElem
	)
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElem);
	Serialize(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElem);
}

// EOF
