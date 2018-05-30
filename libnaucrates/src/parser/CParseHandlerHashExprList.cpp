//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerHashExprList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing hash expression lists.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerHashExprList.h"
#include "naucrates/dxl/parser/CParseHandlerHashExpr.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLScalarProjList.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerHashExprList::CParseHandlerHashExprList
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerHashExprList::CParseHandlerHashExprList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(pmp, parse_handler_mgr, pphRoot)
{
}



//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerHashExprList::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerHashExprList::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarHashExprList), element_local_name))
	{
		// start the hash expr list
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode (m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarHashExprList(m_memory_pool));
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarHashExpr), element_local_name))
	{
		// we must have seen a hash expr list already and initialized the hash expr list node
		GPOS_ASSERT(NULL != m_pdxln);
		// start new hash expr element
		CParseHandlerBase *pphHashExpr = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarHashExpr), m_pphm, this);
		m_pphm->ActivateParseHandler(pphHashExpr);
		
		// store parse handler
		this->Append(pphHashExpr);
		
		pphHashExpr->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerHashExprList::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerHashExprList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarHashExprList), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	const ULONG ulLen = this->Length();
	// add hash expressions from child parse handlers
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CParseHandlerHashExpr *pphHashExpr = dynamic_cast<CParseHandlerHashExpr *>((*this)[ul]);
		
		AddChildFromParseHandler(pphHashExpr);
	}
		
	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
