//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarSwitch.cpp
//
//	@doc:
//
//		Implementation of the SAX parse handler class for a Switch operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarSwitch.h"
#include "naucrates/dxl/parser/CParseHandlerScalarSwitchCase.h"


using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSwitch::CParseHandlerScalarSwitch
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSwitch::CParseHandlerScalarSwitch
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid_type(NULL),
	m_fArgProcessed(false),
	m_fDefaultProcessed(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSwitch::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSwitch::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSwitch), element_local_name) && NULL == m_mdid_type)
	{
		// parse type id
		m_mdid_type = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeId, EdxltokenScalarSwitch);

		// construct node
		CDXLScalarSwitch *dxl_op =  GPOS_NEW(m_memory_pool) CDXLScalarSwitch(m_memory_pool, m_mdid_type);
		m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSwitchCase), element_local_name))
	{
		// we must have already seen the arg child, but have not seen the DEFAULT child
		GPOS_ASSERT(NULL != m_dxl_node && m_fArgProcessed && !m_fDefaultProcessed);

		// parse case
		CParseHandlerBase *pphCase = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSwitchCase), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphCase);

		// store parse handlers
		this->Append(pphCase);

		pphCase->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		GPOS_ASSERT(NULL != m_dxl_node && !m_fDefaultProcessed);

		// parse scalar child
		CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);

		// store parse handlers
		this->Append(child_parse_handler);

		child_parse_handler->startElement(element_uri, element_local_name, element_qname, attrs);

		if (!m_fArgProcessed)
		{
			// this child was the arg child
			m_fArgProcessed = true;
		}
		else
		{
			// that was the default expr child
			m_fDefaultProcessed = true;
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSwitch::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSwitch::EndElement
	(
	const XMLCh* const ,// element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSwitch), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulChildren = this->Length();
	GPOS_ASSERT(1 < ulChildren);

	for (ULONG ul = 0; ul < ulChildren ; ul++)
	{
		CParseHandlerScalarOp *child_parse_handler = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(child_parse_handler);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//EOF
