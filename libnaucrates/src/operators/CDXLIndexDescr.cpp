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
	m_mdid(pmdid),
	m_mdname(pmdname)
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(NULL != m_mdname);
	GPOS_ASSERT(m_mdname->Pstr()->IsValid());
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
	m_mdid->Release();
	GPOS_DELETE(m_mdname);
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
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLIndexDescr::MdName
//
//	@doc:
//		Return index name
//
//---------------------------------------------------------------------------
const CMDName *
CDXLIndexDescr::MdName() const
{
	return m_mdname;
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
	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenIndexName), m_mdname->Pstr());
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenIndexDescr));
}

// EOF
