//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMaterialize.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing materialize operator.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMaterialize.h"
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
//		CParseHandlerMaterialize::CParseHandlerMaterialize
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMaterialize::CParseHandlerMaterialize
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxlop(NULL)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMaterialize::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMaterialize::StartElement
	(
	const XMLCh* const, //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, //element_qname,
	const Attributes& attrs
	)
{

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalMaterialize), element_local_name))
	{
		GPOS_ASSERT(this->Length() == 0 && "No handlers should have been added yet");
	
		m_pdxlop = (CDXLPhysicalMaterialize *) CDXLOperatorFactory::PdxlopMaterialize(m_parse_handler_mgr->Pmm(), attrs);
	
		// parse handler for child node
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);
		
		// parse handler for the filter
		CParseHandlerBase *pphFilter = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphFilter);
	
		// parse handler for the proj list
		CParseHandlerBase *pphPrL = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphPrL);
	
		//parse handler for the properties of the operator
		CParseHandlerBase *pphProp = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphProp);
	
		this->Append(pphProp);
		this->Append(pphPrL);
		this->Append(pphFilter);
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
//		CParseHandlerMaterialize::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMaterialize::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalMaterialize), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	GPOS_ASSERT(4 == this->Length());

	// construct node from the created child nodes	
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerFilter *pphFilter = dynamic_cast<CParseHandlerFilter *>((*this)[2]);
	CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[3]);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_pdxlop);
	// set statictics and physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// add constructed children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);
	AddChildFromParseHandler(pphChild);


#ifdef GPOS_DEBUG
	m_pdxlop->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

