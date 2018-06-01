//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerSequence.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing sequence 
//		operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerSequence.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLPhysicalDynamicTableScan.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSequence::CParseHandlerSequence
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerSequence::CParseHandlerSequence
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_fInsideSequence(false)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSequence::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSequence::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (!m_fInsideSequence && 0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalSequence), element_local_name))
	{
		// new sequence operator
		// parse handler for the proj list
		CParseHandlerBase *pphPrL =
				CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphPrL);

		//parse handler for the properties of the operator
		CParseHandlerBase *pphProp =
				CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphProp);

		// store child parse handlers in array
		this->Append(pphProp);
		this->Append(pphPrL);
		m_fInsideSequence = true;
	}
	else
	{
		// child of the sequence operator
		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, element_local_name, m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);
		this->Append(pphChild);
		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}	
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSequence::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSequence::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalSequence), 
										element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node from the created child nodes
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
		
	CDXLPhysicalSequence *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalSequence(m_memory_pool);
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);	

	// set statistics and physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// add project list
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	GPOS_ASSERT(NULL != pphPrL);
	AddChildFromParseHandler(pphPrL);
			
	const ULONG ulLen = this->Length();
	// add constructed children from child parse handlers
	for (ULONG ul = 2; ul < ulLen; ul++)
	{
		CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[ul]);
		GPOS_ASSERT(NULL != pphChild);
		AddChildFromParseHandler(pphChild);
	}

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG
	
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

