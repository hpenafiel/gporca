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
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_dxl_op(NULL)
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
	
		m_dxl_op = (CDXLPhysicalMaterialize *) CDXLOperatorFactory::PdxlopMaterialize(m_parse_handler_mgr->Pmm(), attrs);
	
		// parse handler for child node
		CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);
		
		// parse handler for the filter
		CParseHandlerBase *filter_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(filter_parse_handler);
	
		// parse handler for the proj list
		CParseHandlerBase *proj_list_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(proj_list_parse_handler);
	
		//parse handler for the properties of the operator
		CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphProp);
	
		this->Append(pphProp);
		this->Append(proj_list_parse_handler);
		this->Append(filter_parse_handler);
		this->Append(child_parse_handler);
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
	CParseHandlerProjList *proj_list_parse_handler = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerFilter *filter_parse_handler = dynamic_cast<CParseHandlerFilter *>((*this)[2]);
	CParseHandlerPhysicalOp *child_parse_handler = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[3]);

	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_dxl_op);
	// set statictics and physical properties
	CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

	// add constructed children
	AddChildFromParseHandler(proj_list_parse_handler);
	AddChildFromParseHandler(filter_parse_handler);
	AddChildFromParseHandler(child_parse_handler);


#ifdef GPOS_DEBUG
	m_dxl_op->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

