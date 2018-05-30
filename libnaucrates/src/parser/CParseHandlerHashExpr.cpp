//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerHashExpr.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing hash expression operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerHashExpr.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerHashExpr::CParseHandlerHashExpr
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerHashExpr::CParseHandlerHashExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(pmp, parse_handler_mgr, pphRoot),
	m_pdxlop(NULL)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerHashExpr::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerHashExpr::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarHashExpr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse and create hash expr operator
	m_pdxlop = (CDXLScalarHashExpr *) CDXLOperatorFactory::PdxlopHashExpr(m_pphm->Pmm(), attrs);
	
	// create and activate the parse handler for the child scalar expression node
	
	CParseHandlerBase *pphOp = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);
	m_pphm->ActivateParseHandler(pphOp);
	
	// store child parse handler
	this->Append(pphOp);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerHashExpr::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerHashExpr::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarHashExpr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	CParseHandlerScalarOp *pphOp = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);
	
	GPOS_ASSERT(NULL != pphOp->Pdxln());
	
	// construct node from the parsed expression node
	m_pdxln = GPOS_NEW(m_pmp) CDXLNode(m_pmp, m_pdxlop);	
			
	AddChildFromParseHandler(pphOp);
	
	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
