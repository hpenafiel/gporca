//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 Pivotal Inc.
//
//	@filename:
//		CParseHandlerScalarMinMax.cpp
//
//	@doc:
//
//		Implementation of the SAX parse handler class for a MinMax operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarMinMax.h"


using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarMinMax::CParseHandlerScalarMinMax
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarMinMax::CParseHandlerScalarMinMax
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid_type(NULL),
	m_emmt(CDXLScalarMinMax::EmmtSentinel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarMinMax::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarMinMax::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (((0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarMin), element_local_name)) ||
		(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarMax), element_local_name))) &&
		CDXLScalarMinMax::EmmtSentinel == m_emmt)
	{
		m_emmt = Emmt(element_local_name);
		GPOS_ASSERT(CDXLScalarMinMax::EmmtSentinel != m_emmt);

		Edxltoken token_type = EdxltokenScalarMin;
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarMax), element_local_name))
		{
			token_type = EdxltokenScalarMax;
		}

		// parse type id
		m_mdid_type = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenTypeId, token_type);
	}
	else
	{
		// parse child
		CParseHandlerBase *pphOp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOp);

		// store parse handlers
		this->Append(pphOp);

		pphOp->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarMinMax::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarMinMax::EndElement
	(
	const XMLCh* const ,// element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	CDXLScalarMinMax::EdxlMinMaxType emmt = Emmt(element_local_name);

	if (CDXLScalarMinMax::EmmtSentinel == emmt || m_emmt != emmt)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarMinMax(m_memory_pool, m_mdid_type, m_emmt));

	// loop over children and add them to this parsehandler
	const ULONG ulChildren = this->Length();
	for (ULONG ul = 0; ul < ulChildren; ul++)
	{
		CParseHandlerScalarOp *child_parse_handler = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(child_parse_handler);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarMinMax::Emmt
//
//	@doc:
//		Parse the min/max type from the attribute value
//
//---------------------------------------------------------------------------
CDXLScalarMinMax::EdxlMinMaxType
CParseHandlerScalarMinMax::Emmt
	(
	const XMLCh *element_local_name
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarMin), element_local_name))
	{
		return CDXLScalarMinMax::EmmtMin;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarMax), element_local_name))
	{
		return CDXLScalarMinMax::EmmtMax;
	}

	return CDXLScalarMinMax::EmmtSentinel;
}


//EOF
