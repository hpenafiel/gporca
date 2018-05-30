//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerArray.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing arrays
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerArray.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerArray::CParseHandlerArray
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerArray::CParseHandlerArray
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
//		CParseHandlerArray::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerArray::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{	
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArray), element_local_name) &&
		NULL == m_pdxln)
	{
		// parse and create array
		CDXLScalarArray *pdxlop = (CDXLScalarArray *) CDXLOperatorFactory::PdxlopArray(m_parse_handler_mgr->Pmm(), attrs);
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlop);
	}
	else
	{
		// parse child of array
		GPOS_ASSERT(NULL != m_pdxln);
		
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);
		this->Append(pphChild);
		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerArray::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerArray::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArray), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// construct node from the created child nodes
	
	GPOS_ASSERT(0 < this->Length());
	
	for (ULONG ul = 0; ul < this->Length(); ul++)
	{
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp*>((*this)[ul]);
		GPOS_ASSERT(NULL != pphChild);
		AddChildFromParseHandler(pphChild);
	}
	
#ifdef GPOS_DEBUG
	m_pdxln->Pdxlop()->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG
	
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
