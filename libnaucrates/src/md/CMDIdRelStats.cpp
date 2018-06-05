//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDIdRelStats.cpp
//
//	@doc:
//		Implementation of mdids for relation statistics
//---------------------------------------------------------------------------


#include "naucrates/md/CMDIdRelStats.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::CMDIdRelStats
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdRelStats::CMDIdRelStats
	(
	CMDIdGPDB *pmdidRel
	)
	:
	m_rel_mdid(pmdidRel),
	m_str(m_wszBuffer, GPOS_ARRAY_SIZE(m_wszBuffer))
{
	// serialize mdid into static string 
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::~CMDIdRelStats
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIdRelStats::~CMDIdRelStats()
{
	m_rel_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::Serialize
//
//	@doc:
//		Serialize mdid into static string
//
//---------------------------------------------------------------------------
void
CMDIdRelStats::Serialize()
{
	// serialize mdid as SystemType.Oid.Major.Minor
	m_str.AppendFormat
			(
			GPOS_WSZ_LIT("%d.%d.%d.%d"), 
			Emdidt(), 
			m_rel_mdid->OidObjectId(),
			m_rel_mdid->UlVersionMajor(),
			m_rel_mdid->UlVersionMinor()
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::GetBuffer
//
//	@doc:
//		Returns the string representation of the mdid
//
//---------------------------------------------------------------------------
const WCHAR *
CMDIdRelStats::GetBuffer() const
{
	return m_str.GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::PmdidRel
//
//	@doc:
//		Returns the base relation id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdRelStats::PmdidRel() const
{
	return m_rel_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::Equals
//
//	@doc:
//		Checks if the mdids are equal
//
//---------------------------------------------------------------------------
BOOL
CMDIdRelStats::Equals
	(
	const IMDId *pmdid
	) 
	const
{
	if (NULL == pmdid || EmdidRelStats != pmdid->Emdidt())
	{
		return false;
	}
	
	const CMDIdRelStats *pmdidRelStats = CMDIdRelStats::PmdidConvert(pmdid);
	
	return m_rel_mdid->Equals(pmdidRelStats->PmdidRel()); 
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdRelStats::Serialize
//
//	@doc:
//		Serializes the mdid as the value of the given attribute
//
//---------------------------------------------------------------------------
void
CMDIdRelStats::Serialize
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
//		CMDIdRelStats::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdRelStats::OsPrint
	(
	IOstream &os
	) 
	const
{
	os << "(" << m_str.GetBuffer() << ")";
	return os;
}

// EOF
