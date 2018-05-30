//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarCoalesce.cpp
//
//	@doc:
//
//		Implementation of the SAX parse handler class for a coalesce operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarCoalesce.h"


using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarCoalesce::CParseHandlerScalarCoalesce
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarCoalesce::CParseHandlerScalarCoalesce
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, pphRoot),
	m_mdid_type(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarCoalesce::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarCoalesce::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarCoalesce), element_local_name) && NULL == m_mdid_type)
	{
		// parse type id
		m_mdid_type = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeId, EdxltokenScalarCoalesce);
	}
	else
	{
		// parse scalar child
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handlers
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarCoalesce::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarCoalesce::EndElement
	(
	const XMLCh* const ,// element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarCoalesce), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarCoalesce(m_memory_pool, m_mdid_type));

	// loop over children and add them to this parsehandler
	const ULONG ulChildren = this->Length();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(pphChild);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//EOF
