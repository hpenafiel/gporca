//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerScalarPartBoundOpen.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing part bound openness
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarPartBoundOpen.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLScalarPartBoundOpen.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarPartBoundOpen::CParseHandlerScalarPartBoundOpen
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarPartBoundOpen::CParseHandlerScalarPartBoundOpen
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, pphRoot)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarPartBoundOpen::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarPartBoundOpen::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPartBoundOpen), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	ULONG ulLevel = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPartLevel, EdxltokenScalarPartBoundOpen);
	BOOL fLower = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenScalarPartBoundLower, EdxltokenScalarPartBoundOpen);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode (m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarPartBoundOpen(m_memory_pool, ulLevel, fLower));
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarPartBoundOpen::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarPartBoundOpen::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPartBoundOpen), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxln);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
