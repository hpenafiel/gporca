//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal Inc.
//
//	@filename:
//		CParseHandlerDirectDispatchInfo.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing direct dispatch info
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerDirectDispatchInfo.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"

using namespace gpdxl;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::CParseHandlerDirectDispatchInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerDirectDispatchInfo::CParseHandlerDirectDispatchInfo
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdrgpdxldatum(NULL),
	m_pdrgpdrgpdxldatum(NULL),
	m_pdxlddinfo(NULL)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::~CParseHandlerDirectDispatchInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerDirectDispatchInfo::~CParseHandlerDirectDispatchInfo()
{
	CRefCount::SafeRelease(m_pdrgpdxldatum);
	CRefCount::SafeRelease(m_pdxlddinfo);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDirectDispatchInfo::StartElement
	(
	const XMLCh* const , //xmlstrUri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const , // xmlstrQname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), xmlstrLocalname))
	{
		m_pdrgpdrgpdxldatum = GPOS_NEW(m_memory_pool) DXLDatumArrays(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchKeyValue), xmlstrLocalname))
	{
		CRefCount::SafeRelease(m_pdrgpdxldatum);
		m_pdrgpdxldatum = GPOS_NEW(m_memory_pool) DXLDatumArray(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDatum), xmlstrLocalname))
	{
		GPOS_ASSERT(NULL != m_pdrgpdxldatum);

		CDXLDatum *datum_dxl = CDXLOperatorFactory::Pdxldatum(m_pphm->Pmm(), attrs, EdxltokenDirectDispatchInfo);
		m_pdrgpdxldatum->Append(datum_dxl);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDirectDispatchInfo::EndElement
	(
	const XMLCh* const, // xmlstrUri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const // xmlstrQname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), xmlstrLocalname))
	{
		m_pdxlddinfo = GPOS_NEW(m_memory_pool) CDXLDirectDispatchInfo(m_pdrgpdrgpdxldatum);
		m_pphm->DeactivateHandler();
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchKeyValue), xmlstrLocalname))
	{
		GPOS_ASSERT(NULL != m_pdrgpdxldatum);
		m_pdrgpdxldatum->AddRef();
		m_pdrgpdrgpdxldatum->Append(m_pdrgpdxldatum);
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDatum), xmlstrLocalname))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::Pdxlddinfo
//
//	@doc:
//		Return parsed DXL datum array
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo *
CParseHandlerDirectDispatchInfo::Pdxlddinfo() const
{
	return m_pdxlddinfo;
}

// EOF

