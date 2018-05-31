//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerCostParam
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing xform
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerCostParam.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

using namespace gpdxl;
using namespace gpopt;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParam::CParseHandlerCostParam
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerCostParam::CParseHandlerCostParam
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_szName(NULL),
	m_dVal(0),
	m_dLowerBound(0),
	m_dUpperBound(0)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParam::~CParseHandlerCostParam
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerCostParam::~CParseHandlerCostParam()
{
	GPOS_DELETE_ARRAY(m_szName);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParam::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCostParam::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const, // xmlstrQname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCostParam), xmlstrLocalname))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const XMLCh *xmlstrName = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenName, EdxltokenCostParam);
	CWStringDynamic *pstrName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrName);
	m_szName = CDXLUtils::CreateMultiByteCharStringFromWCString(m_memory_pool, pstrName->GetBuffer());
	GPOS_DELETE(pstrName);

	m_dVal = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenValue, EdxltokenCostParam);
	m_dLowerBound = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCostParamLowerBound, EdxltokenCostParam);
	m_dUpperBound = CDXLOperatorFactory::DValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCostParamUpperBound, EdxltokenCostParam);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParam::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCostParam::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const // xmlstrQname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCostParam), xmlstrLocalname))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}


// EOF

