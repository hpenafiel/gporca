//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CParseHandlerWindowOids.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing window oids
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerWindowOids.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;
using namespace gpopt;

XERCES_CPP_NAMESPACE_USE

CParseHandlerWindowOids::CParseHandlerWindowOids
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pwindowoids(NULL)
{
}

CParseHandlerWindowOids::~CParseHandlerWindowOids()
{
	CRefCount::SafeRelease(m_pwindowoids);
}

void
CParseHandlerWindowOids::StartElement
	(
	const XMLCh* const , //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , //element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowOids), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// parse window function oids
	OID oidRowNumber = CDXLOperatorFactory::ExtractConvertAttrValueToOid(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenOidRowNumber, EdxltokenWindowOids);
	OID oidRank = CDXLOperatorFactory::ExtractConvertAttrValueToOid(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenOidRank, EdxltokenWindowOids);

	m_pwindowoids = GPOS_NEW(m_memory_pool) CWindowOids(oidRowNumber, oidRank);
}

// invoked by Xerces to process a closing tag
void
CParseHandlerWindowOids::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowOids), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE( gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pwindowoids);
	GPOS_ASSERT(0 == this->Length());

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// return the type of the parse handler.
EDxlParseHandlerType
CParseHandlerWindowOids::GetParseHandlerType() const
{
	return EdxlphWindowOids;
}

CWindowOids *
CParseHandlerWindowOids::Pwindowoids() const
{
	return m_pwindowoids;
}

// EOF
