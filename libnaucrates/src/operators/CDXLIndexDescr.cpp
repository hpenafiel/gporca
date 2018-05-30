//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLIndexDescr.cpp
//
//	@doc:
//		Implementation of DXL index descriptors
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLIndexDescr::CDXLIndexDescr
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLIndexDescr::CDXLIndexDescr
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *pmdname
	)
	:
	m_memory_pool(memory_pool),
	m_pmdid(pmdid),
	m_pmdname(pmdname)
{
	GPOS_ASSERT(m_pmdid->IsValid());
	GPOS_ASSERT(NULL != m_pmdname);
	GPOS_ASSERT(m_pmdname->Pstr()->IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLIndexDescr::~CDXLIndexDescr
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLIndexDescr::~CDXLIndexDescr()
{
	m_pmdid->Release();
	GPOS_DELETE(m_pmdname);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLIndexDescr::MDId
//
//	@doc:
//		Return the metadata id for the index
//
//---------------------------------------------------------------------------
IMDId *
CDXLIndexDescr::MDId() const
{
	return m_pmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLIndexDescr::Pmdname
//
//	@doc:
//		Return index name
//
//---------------------------------------------------------------------------
const CMDName *
CDXLIndexDescr::Pmdname() const
{
	return m_pmdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLIndexDescr::SerializeToDXL
//
//	@doc:
//		Serialize index descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLIndexDescr::SerializeToDXL
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenIndexDescr));
	m_pmdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIndexName), m_pmdname->Pstr());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenIndexDescr));
}

// EOF
