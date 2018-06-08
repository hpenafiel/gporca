//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDIdScCmp.cpp
//
//	@doc:
//		Implementation of mdids for scalar comparisons functions
//---------------------------------------------------------------------------

#include "naucrates/md/CMDIdScCmp.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::CMDIdScCmp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdScCmp::CMDIdScCmp
	(
	CMDIdGPDB *pmdidLeft,
	CMDIdGPDB *pmdidRight,
	IMDType::ECmpType ecmpt
	)
	:
	m_pmdidLeft(pmdidLeft),
	m_pmdidRight(pmdidRight),
	m_comparision_type(ecmpt),
	m_str(m_wszBuffer, GPOS_ARRAY_SIZE(m_wszBuffer))
{
	GPOS_ASSERT(pmdidLeft->IsValid());
	GPOS_ASSERT(pmdidRight->IsValid());
	GPOS_ASSERT(IMDType::EcmptOther != ecmpt);
	
	GPOS_ASSERT(pmdidLeft->Sysid().Equals(pmdidRight->Sysid()));
	
	// serialize mdid into static string 
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::~CMDIdScCmp
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIdScCmp::~CMDIdScCmp()
{
	m_pmdidLeft->Release();
	m_pmdidRight->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::Serialize
//
//	@doc:
//		Serialize mdid into static string
//
//---------------------------------------------------------------------------
void
CMDIdScCmp::Serialize()
{
	// serialize mdid as SystemType.mdidLeft;mdidRight;CmpType
	m_str.AppendFormat
			(
			GPOS_WSZ_LIT("%d.%d.%d.%d;%d.%d.%d;%d"), 
			Emdidt(), 
			m_pmdidLeft->OidObjectId(),
			m_pmdidLeft->UlVersionMajor(),
			m_pmdidLeft->UlVersionMinor(),
			m_pmdidRight->OidObjectId(),
			m_pmdidRight->UlVersionMajor(),
			m_pmdidRight->UlVersionMinor(),
			m_comparision_type
			);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::GetBuffer
//
//	@doc:
//		Returns the string representation of the mdid
//
//---------------------------------------------------------------------------
const WCHAR *
CMDIdScCmp::GetBuffer() const
{
	return m_str.GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::PmdidLeft
//
//	@doc:
//		Returns the source type id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdScCmp::PmdidLeft() const
{
	return m_pmdidLeft;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::PmdidRight
//
//	@doc:
//		Returns the destination type id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdScCmp::PmdidRight() const
{
	return m_pmdidRight;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::HashValue
//
//	@doc:
//		Computes the hash value for the metadata id
//
//---------------------------------------------------------------------------
ULONG
CMDIdScCmp::HashValue() const
{
	return gpos::CombineHashes
								(
								Emdidt(), 
								gpos::CombineHashes(m_pmdidLeft->HashValue(), m_pmdidRight->HashValue())
								);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::Equals
//
//	@doc:
//		Checks if the mdids are equal
//
//---------------------------------------------------------------------------
BOOL
CMDIdScCmp::Equals
	(
	const IMDId *mdid
	) 
	const
{
	if (NULL == mdid || EmdidScCmp != mdid->Emdidt())
	{
		return false;
	}
	
	const CMDIdScCmp *pmdidScCmp = CMDIdScCmp::PmdidConvert(mdid);
	
	return m_pmdidLeft->Equals(pmdidScCmp->PmdidLeft()) && 
			m_pmdidRight->Equals(pmdidScCmp->PmdidRight()) &&
			m_comparision_type == pmdidScCmp->ParseCmpType(); 
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdScCmp::Serialize
//
//	@doc:
//		Serializes the mdid as the value of the given attribute
//
//---------------------------------------------------------------------------
void
CMDIdScCmp::Serialize
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
//		CMDIdScCmp::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdScCmp::OsPrint
	(
	IOstream &os
	) 
	const
{
	os << "(" << m_str.GetBuffer() << ")";
	return os;
}

// EOF
