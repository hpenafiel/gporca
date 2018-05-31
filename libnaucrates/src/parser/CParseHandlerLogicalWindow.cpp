//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalWindow.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		window operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalWindow.h"
#include "naucrates/dxl/parser/CParseHandlerWindowSpecList.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalWindow::CParseHandlerLogicalWindow
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalWindow::CParseHandlerLogicalWindow
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pdrgpdxlws(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalWindow::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalWindow::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& //attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalWindow), element_local_name))
	{
		// create child node parsers
		// parse handler for logical operator
		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// parse handler for the proj list
		CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphPrL);

		// parse handler for window specification list
		CParseHandlerBase *pphWsL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenWindowSpecList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphWsL);

		// store child parse handler in array
		this->Append(pphWsL);
		this->Append(pphPrL);
		this->Append(pphChild);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalWindow::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalWindow::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalWindow), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerWindowSpecList *pphWsL = dynamic_cast<CParseHandlerWindowSpecList*>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerLogicalOp *pphLgOp = dynamic_cast<CParseHandlerLogicalOp*>((*this)[2]);

	DXLWindowSpecArray *pdrgpdxlws = pphWsL->Pdrgpdxlws();
	GPOS_ASSERT(NULL != pdrgpdxlws);

	CDXLLogicalWindow *pdxlopWin = GPOS_NEW(m_memory_pool) CDXLLogicalWindow(m_memory_pool, pdrgpdxlws);
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopWin);
	GPOS_ASSERT(NULL != pphPrL->Pdxln());
	GPOS_ASSERT(NULL != pphLgOp->Pdxln());

	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphLgOp);

#ifdef GPOS_DEBUG
	m_pdxln->Pdxlop()->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
