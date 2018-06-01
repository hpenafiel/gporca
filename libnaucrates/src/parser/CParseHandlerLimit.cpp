//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerLimit.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing Limit operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLimit.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLimit::CParseHandlerLimit
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerLimit::CParseHandlerLimit
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
//		CParseHandlerLimit::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLimit::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalLimit), element_local_name))
	{

		// parse and create Limit operator
		m_dxl_op = (CDXLPhysicalLimit *) CDXLOperatorFactory::PdxlopLimit(m_parse_handler_mgr->Pmm(), attrs);
		m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_dxl_op);

		// create and activate the parse handler for the children nodes in reverse
		// order of their expected appearance

		CParseHandlerBase *pphOffset = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarLimitOffset), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOffset);

		CParseHandlerBase *pphCount = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarLimitCount), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphCount);

		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// parse handler for the proj list
		CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphPrL);

		//parse handler for the properties of the operator
		CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphProp);

		// store parse handlers
		this->Append(pphProp);
		this->Append(pphPrL);
		this->Append(pphChild);
		this->Append(pphCount);
		this->Append(pphOffset);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLimit::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLimit::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalLimit), element_local_name))
	{
		GPOS_ASSERT(5 == this->Length());

		// construct node from the created child nodes
		CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);

		CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
		CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[2]);
		CParseHandlerScalarOp *pphCount = dynamic_cast<CParseHandlerScalarOp *>((*this)[3]);
		CParseHandlerScalarOp *pphOffSet = dynamic_cast<CParseHandlerScalarOp *>((*this)[4]);
		
		// set statistics and physical properties
		CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

		// add constructed children
		AddChildFromParseHandler(pphPrL);
		AddChildFromParseHandler(pphChild);
		AddChildFromParseHandler(pphCount);
		AddChildFromParseHandler(pphOffSet);

		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

// EOF
