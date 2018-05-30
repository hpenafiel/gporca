//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerScalarPartBoundInclusion.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing part bound inclusion
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarPartBoundInclusion.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLScalarPartBoundInclusion.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarPartBoundInclusion::CParseHandlerScalarPartBoundInclusion
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarPartBoundInclusion::CParseHandlerScalarPartBoundInclusion
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
//		CParseHandlerScalarPartBoundInclusion::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarPartBoundInclusion::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPartBoundInclusion), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	ULONG ulLevel = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPartLevel, EdxltokenScalarPartBoundInclusion);
	BOOL fLower = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenScalarPartBoundLower, EdxltokenScalarPartBoundInclusion);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode (m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarPartBoundInclusion(m_memory_pool, ulLevel, fLower));
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarPartBoundInclusion::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarPartBoundInclusion::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPartBoundInclusion), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxln);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
