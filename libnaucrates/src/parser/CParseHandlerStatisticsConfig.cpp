//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerStatisticsConfig.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing statistics
//		configuration
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerStatisticsConfig.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/xml/dxltokens.h"

#include "gpopt/engine/CStatisticsConfig.h"

using namespace gpdxl;
using namespace gpopt;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatisticsConfig::CParseHandlerStatisticsConfig
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerStatisticsConfig::CParseHandlerStatisticsConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pstatsconf(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatisticsConfig::~CParseHandlerStatisticsConfig
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerStatisticsConfig::~CParseHandlerStatisticsConfig()
{
	CRefCount::SafeRelease(m_pstatsconf);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatisticsConfig::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerStatisticsConfig::StartElement
	(
	const XMLCh* const , //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , //element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatisticsConfig), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// parse statistics configuration options
	CDouble dDampingFactorFilter = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenDampingFactorFilter, EdxltokenStatisticsConfig);
	CDouble dDampingFactorJoin = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenDampingFactorJoin, EdxltokenStatisticsConfig);
	CDouble dDampingFactorGroupBy = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenDampingFactorGroupBy, EdxltokenStatisticsConfig);

	m_pstatsconf = GPOS_NEW(m_memory_pool) CStatisticsConfig(m_memory_pool, dDampingFactorFilter, dDampingFactorJoin, dDampingFactorGroupBy);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatisticsConfig::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerStatisticsConfig::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatisticsConfig), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE( gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pstatsconf);
	GPOS_ASSERT(0 == this->Length());

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatisticsConfig::GetParseHandlerType
//
//	@doc:
//		Return the type of the parse handler.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerStatisticsConfig::GetParseHandlerType() const
{
	return EdxlphStatisticsConfig;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatisticsConfig::Pstatsconf
//
//	@doc:
//		Returns the statistics configuration
//
//---------------------------------------------------------------------------
CStatisticsConfig *
CParseHandlerStatisticsConfig::Pstatsconf() const
{
	return m_pstatsconf;
}

// EOF
