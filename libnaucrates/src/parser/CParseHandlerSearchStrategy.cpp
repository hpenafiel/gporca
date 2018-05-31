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
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_search_stage_array(NULL)
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
	CRefCount::SafeRelease(m_search_stage_array);
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
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStrategy), element_local_name))
	{
		m_search_stage_array = GPOS_NEW(m_memory_pool) DrgPss(m_memory_pool);
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStage), element_local_name))
	{
		GPOS_ASSERT(NULL != m_search_stage_array);

		// start new search stage
		CParseHandlerBase *pphSearchStage = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenSearchStage), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphSearchStage);

		// store parse handler
		this->Append(pphSearchStage);

		pphSearchStage->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
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
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStrategy), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerSearchStage *pphSearchStage = dynamic_cast<CParseHandlerSearchStage*>((*this)[ul]);
		CXformSet *pxfs = pphSearchStage->Pxfs();
		pxfs->AddRef();
		CSearchStage *pss = GPOS_NEW(m_memory_pool) CSearchStage(pxfs, pphSearchStage->UlTimeThreshold(), pphSearchStage->CostThreshold());
		m_search_stage_array->Append(pss);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

