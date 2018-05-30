//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerSearchStrategy.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing search strategy.
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerSearchStrategy.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerSearchStage.h"

using namespace gpdxl;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStrategy::CParseHandlerSearchStrategy
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerSearchStrategy::CParseHandlerSearchStrategy
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdrgpss(NULL)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStrategy::~CParseHandlerSearchStrategy
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerSearchStrategy::~CParseHandlerSearchStrategy()
{
	CRefCount::SafeRelease(m_pdrgpss);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStrategy::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSearchStrategy::StartElement
	(
	const XMLCh* const xmlstrUri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const xmlstrQname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStrategy), xmlstrLocalname))
	{
		m_pdrgpss = GPOS_NEW(m_memory_pool) DrgPss(m_memory_pool);
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStage), xmlstrLocalname))
	{
		GPOS_ASSERT(NULL != m_pdrgpss);

		// start new search stage
		CParseHandlerBase *pphSearchStage = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenSearchStage), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphSearchStage);

		// store parse handler
		this->Append(pphSearchStage);

		pphSearchStage->startElement(xmlstrUri, xmlstrLocalname, xmlstrQname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStrategy::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSearchStrategy::EndElement
	(
	const XMLCh* const, // xmlstrUri,
	const XMLCh* const xmlstrLocalname,
	const XMLCh* const // xmlstrQname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStrategy), xmlstrLocalname))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlstrLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerSearchStage *pphSearchStage = dynamic_cast<CParseHandlerSearchStage*>((*this)[ul]);
		CXformSet *pxfs = pphSearchStage->Pxfs();
		pxfs->AddRef();
		CSearchStage *pss = GPOS_NEW(m_memory_pool) CSearchStage(pxfs, pphSearchStage->UlTimeThreshold(), pphSearchStage->CostThreshold());
		m_pdrgpss->Append(pss);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

