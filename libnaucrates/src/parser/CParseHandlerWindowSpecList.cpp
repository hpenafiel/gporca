//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerWindowSpecList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the list of
//		window specifications in the logical window operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerWindowSpec.h"
#include "naucrates/dxl/parser/CParseHandlerWindowSpecList.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowSpecList::CParseHandlerWindowSpecList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerWindowSpecList::CParseHandlerWindowSpecList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdrgpdxlws(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowSpecList::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowSpecList::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowSpecList), element_local_name))
	{
		m_pdrgpdxlws = GPOS_NEW(m_memory_pool) DrgPdxlws(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowSpec), element_local_name))
	{
		// we must have seen a window specification list already
		GPOS_ASSERT(NULL != m_pdrgpdxlws);
		// start new window specification element
		CParseHandlerBase *pphWs =
				CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenWindowSpec), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphWs);

		// store parse handler
		this->Append(pphWs);

		pphWs->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowSpecList::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowSpecList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowSpecList), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL != m_pdrgpdxlws);

	const ULONG ulSize = this->Length();
	// add the window specifications to the list
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerWindowSpec *pphWs = dynamic_cast<CParseHandlerWindowSpec *>((*this)[ul]);
		m_pdrgpdxlws->Append(pphWs->Pdxlws());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
