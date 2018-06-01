//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMergeJoin.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing merge join operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMergeJoin.h"
#include "naucrates/dxl/parser/CParseHandlerCondList.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMergeJoin::CParseHandlerMergeJoin
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMergeJoin::CParseHandlerMergeJoin
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_dxl_op(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMergeJoin::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMergeJoin::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalMergeJoin), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse and create Merge join operator
	m_dxl_op = (CDXLPhysicalMergeJoin *) CDXLOperatorFactory::PdxlopMergeJoin(m_parse_handler_mgr->Pmm(), attrs);
	
	// create and activate the parse handler for the children nodes in reverse
	// order of their expected appearance
	
	// parse handler for right child
	CParseHandlerBase *pphRight = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphRight);
	
	// parse handler for left child
	CParseHandlerBase *pphLeft = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphLeft);

	// parse handler for the merge clauses
	CParseHandlerBase *pphMergeCl = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarMergeCondList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphMergeCl);
	
	// parse handler for the join filter
	CParseHandlerBase *ppHjFilter = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(ppHjFilter);
	
	// parse handler for the filter
	CParseHandlerBase *pphFilter = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphFilter);
	
	// parse handler for the proj list
	CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);
	
	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);
	
	// store parse handlers
	this->Append(pphProp);
	this->Append(pphPrL);
	this->Append(pphFilter);
	this->Append(ppHjFilter);
	this->Append(pphMergeCl);
	this->Append(pphLeft);
	this->Append(pphRight);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMergeJoin::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMergeJoin::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalMergeJoin), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// construct node from the created child nodes
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerFilter *pphFilter = dynamic_cast<CParseHandlerFilter *>((*this)[2]);
	CParseHandlerFilter *ppHjFilter = dynamic_cast<CParseHandlerFilter *>((*this)[3]);
	CParseHandlerCondList *pphMergeCl = dynamic_cast<CParseHandlerCondList *>((*this)[4]);
	CParseHandlerPhysicalOp *pphLeft = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[5]);
	CParseHandlerPhysicalOp *pphRight = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[6]);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_dxl_op);	
	// set statictics and physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// add children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);
	AddChildFromParseHandler(ppHjFilter);
	AddChildFromParseHandler(pphMergeCl);
	AddChildFromParseHandler(pphLeft);
	AddChildFromParseHandler(pphRight);
	
#ifdef GPOS_DEBUG
	m_dxl_op->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
