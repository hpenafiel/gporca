//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDIdCast.cpp
//
//	@doc:
//		Implementation of mdids for cast functions
//---------------------------------------------------------------------------


#include "naucrates/md/CMDIdCast.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::CMDIdCast
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdCast::CMDIdCast
	(
	CMDIdGPDB *mdid_src,
	CMDIdGPDB *mdid_dest
	)
	:
	m_pmdidSrc(mdid_src),
	m_pmdidDest(mdid_dest),
	m_str(m_wszBuffer, GPOS_ARRAY_SIZE(m_wszBuffer))
{
	GPOS_ASSERT(mdid_src->IsValid());
	GPOS_ASSERT(mdid_dest->IsValid());
	
	// serialize mdid into static string 
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::~CMDIdCast
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIdCast::~CMDIdCast()
{
	m_pmdidSrc->Release();
	m_pmdidDest->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::Serialize
//
//	@doc:
//		Serialize mdid into static string
//
//---------------------------------------------------------------------------
void
CMDIdCast::Serialize()
{
	// serialize mdid as SystemType.mdidSrc.mdidDest
	m_str.AppendFormat
			(
			GPOS_WSZ_LIT("%d.%d.%d.%d;%d.%d.%d"), 
			Emdidt(), 
			m_pmdidSrc->OidObjectId(),
			m_pmdidSrc->UlVersionMajor(),
			m_pmdidSrc->UlVersionMinor(),
			m_pmdidDest->OidObjectId(),
			m_pmdidDest->UlVersionMajor(),
			m_pmdidDest->UlVersionMinor()			
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::GetBuffer
//
//	@doc:
//		Returns the string representation of the mdid
//
//---------------------------------------------------------------------------
const WCHAR *
CMDIdCast::GetBuffer() const
{
	return m_str.GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::PmdidSrc
//
//	@doc:
//		Returns the source type id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdCast::PmdidSrc() const
{
	return m_pmdidSrc;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::PmdidDest
//
//	@doc:
//		Returns the destination type id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdCast::PmdidDest() const
{
	return m_pmdidDest;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::Equals
//
//	@doc:
//		Checks if the mdids are equal
//
//---------------------------------------------------------------------------
BOOL
CMDIdCast::Equals
	(
	const IMDId *mdid
	) 
	const
{
	if (NULL == mdid || EmdidCastFunc != mdid->Emdidt())
	{
		return false;
	}
	
	const CMDIdCast *mdid_cast_func = CMDIdCast::PmdidConvert(mdid);
	
	return m_pmdidSrc->Equals(mdid_cast_func->PmdidSrc()) && 
			m_pmdidDest->Equals(mdid_cast_func->PmdidDest()); 
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::Serialize
//
//	@doc:
//		Serializes the mdid as the value of the given attribute
//
//---------------------------------------------------------------------------
void
CMDIdCast::Serialize
	(
	CXMLSerializer * xml_serializer,
	const CWStringConst *pstrAttribute
	)
	const
{
	xml_serializer->AddAttribute(pstrAttribute, &m_str);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdCast::OsPrint
	(
	IOstream &os
	) 
	const
{
	os << "(" << m_str.GetBuffer() << ")";
	return os;
}

// EOF
