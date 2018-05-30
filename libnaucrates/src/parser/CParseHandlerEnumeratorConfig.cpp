//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerEnumeratorConfig.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing enumerator
//		configuratiom params
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerEnumeratorConfig.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/xml/dxltokens.h"

#include "gpopt/engine/CEnumeratorConfig.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerEnumeratorConfig::CParseHandlerEnumeratorConfig
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerEnumeratorConfig::CParseHandlerEnumeratorConfig
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pec(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerEnumeratorConfig::~CParseHandlerEnumeratorConfig
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerEnumeratorConfig::~CParseHandlerEnumeratorConfig()
{
	CRefCount::SafeRelease(m_pec);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerEnumeratorConfig::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerEnumeratorConfig::StartElement
	(
	const XMLCh* const , //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , //element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenEnumeratorConfig), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// parse enumerator config options
	ULLONG plan_id = CDXLOperatorFactory::UllValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPlanId, EdxltokenOptimizerConfig);
	ULLONG ullPlanSamples = CDXLOperatorFactory::UllValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPlanSamples, EdxltokenOptimizerConfig);
	CDouble dCostThreshold = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCostThreshold, EdxltokenOptimizerConfig);

	m_pec = GPOS_NEW(m_memory_pool) CEnumeratorConfig(m_memory_pool, plan_id, ullPlanSamples, dCostThreshold);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerEnumeratorConfig::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerEnumeratorConfig::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenEnumeratorConfig), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE( gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pec);
	GPOS_ASSERT(0 == this->Length());

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerEnumeratorConfig::Edxlphtype
//
//	@doc:
//		Return the type of the parse handler.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerEnumeratorConfig::Edxlphtype() const
{
	return EdxlphEnumeratorConfig;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerEnumeratorConfig::Pec
//
//	@doc:
//		Returns the enumerator configuration
//
//---------------------------------------------------------------------------
CEnumeratorConfig *
CParseHandlerEnumeratorConfig::Pec() const
{
	return m_pec;
}

// EOF
