//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerCostParams.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing cost parameters.
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerCostParam.h"
#include "naucrates/dxl/parser/CParseHandlerCostParams.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "gpdbcost/CCostModelParamsGPDB.h"

using namespace gpdxl;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParams::CParseHandlerCostParams
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerCostParams::CParseHandlerCostParams
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pcp(NULL)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParams::~CParseHandlerCostParams
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerCostParams::~CParseHandlerCostParams()
{
	CRefCount::SafeRelease(m_pcp);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParams::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCostParams::StartElement
	(
	const XMLCh* const xmlstrUri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const xmlstrQname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCostParams), xmlstrLocalname))
	{
		// as of now, we only parse params of GPDB cost model
		m_pcp = GPOS_NEW(m_memory_pool) CCostModelParamsGPDB(m_memory_pool);
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCostParam), xmlstrLocalname))
	{
		GPOS_ASSERT(NULL != m_pcp);

		// start new search stage
		CParseHandlerBase *pphCostParam = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCostParam), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphCostParam);

		// store parse handler
		this->Append(pphCostParam);

		pphCostParam->startElement(xmlstrUri, xmlstrLocalname, xmlstrQname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCostParams::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCostParams::EndElement
	(
	const XMLCh* const, // xmlstrUri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const // xmlstrQname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCostParams), xmlstrLocalname))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerCostParam *pphCostParam = dynamic_cast<CParseHandlerCostParam*>((*this)[ul]);
		m_pcp->SetParam(pphCostParam->GetName(), pphCostParam->Get(), pphCostParam->GetLowerBoundVal(), pphCostParam->GetUpperBoundVal());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

