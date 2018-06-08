//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDIdColStats.cpp
//
//	@doc:
//		Implementation of mdids for column statistics
//---------------------------------------------------------------------------


#include "naucrates/md/CMDIdColStats.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::CMDIdColStats
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdColStats::CMDIdColStats
	(
	CMDIdGPDB *rel_mdid,
	ULONG ulPos
	)
	:
	m_rel_mdid(rel_mdid),
	m_ulPos(ulPos),
	m_str(m_wszBuffer, GPOS_ARRAY_SIZE(m_wszBuffer))
{
	GPOS_ASSERT(rel_mdid->IsValid());
	
	// serialize mdid into static string 
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::~CMDIdColStats
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIdColStats::~CMDIdColStats()
{
	m_rel_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::Serialize
//
//	@doc:
//		Serialize mdid into static string
//
//---------------------------------------------------------------------------
void
CMDIdColStats::Serialize()
{
	// serialize mdid as SystemType.Oid.Major.Minor.Attno
	m_str.AppendFormat
			(
			GPOS_WSZ_LIT("%d.%d.%d.%d.%d"), 
			Emdidt(), 
			m_rel_mdid->OidObjectId(),
			m_rel_mdid->UlVersionMajor(),
			m_rel_mdid->UlVersionMinor(),
			m_ulPos
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::GetBuffer
//
//	@doc:
//		Returns the string representation of the mdid
//
//---------------------------------------------------------------------------
const WCHAR *
CMDIdColStats::GetBuffer() const
{
	return m_str.GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::GetRelMdId
//
//	@doc:
//		Returns the base relation id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdColStats::GetRelMdId() const
{
	return m_rel_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::UlPos
//
//	@doc:
//		Returns the attribute number
//
//---------------------------------------------------------------------------
ULONG
CMDIdColStats::UlPos() const
{
	return m_ulPos;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::Equals
//
//	@doc:
//		Checks if the mdids are equal
//
//---------------------------------------------------------------------------
BOOL
CMDIdColStats::Equals
	(
	const IMDId *mdid
	) 
	const
{
	if (NULL == mdid || EmdidColStats != mdid->Emdidt())
	{
		return false;
	}
	
	const CMDIdColStats *mdid_col_stats = CMDIdColStats::PmdidConvert(mdid);
	
	return m_rel_mdid->Equals(mdid_col_stats->GetRelMdId()) && 
			m_ulPos == mdid_col_stats->UlPos(); 
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdColStats::Serialize
//
//	@doc:
//		Serializes the mdid as the value of the given attribute
//
//---------------------------------------------------------------------------
void
CMDIdColStats::Serialize
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
//		CMDIdColStats::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdColStats::OsPrint
	(
	IOstream &os
	) 
	const
{
	os << "(" << m_str.GetBuffer() << ")";
	return os;
}

// EOF
