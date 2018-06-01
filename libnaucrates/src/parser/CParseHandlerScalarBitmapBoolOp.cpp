//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CParseHandlerScalarBitmapBoolOp.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar bitmap
//		bool op
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarBitmapBoolOp.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBitmapBoolOp::CParseHandlerScalarBitmapBoolOp
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarBitmapBoolOp::CParseHandlerScalarBitmapBoolOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBitmapBoolOp::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBitmapBoolOp::StartElement
	(
	const XMLCh* const , // element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	CDXLScalarBitmapBoolOp::EdxlBitmapBoolOp edxlbitmapboolop = CDXLScalarBitmapBoolOp::EdxlbitmapAnd;
	Edxltoken token_type = EdxltokenScalarBitmapAnd;
	
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBitmapOr), element_local_name))
	{
		edxlbitmapboolop = CDXLScalarBitmapBoolOp::EdxlbitmapOr;
		token_type = EdxltokenScalarBitmapOr;
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBitmapAnd), element_local_name))
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name)->GetBuffer());
	}
	
	IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeId, token_type);
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarBitmapBoolOp(m_memory_pool, pmdid, edxlbitmapboolop));
	
	// install parse handlers for children
	CParseHandlerBase *pphRight = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphRight);
	
	CParseHandlerBase *pphLeft = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphLeft);

	this->Append(pphLeft);
	this->Append(pphRight);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBitmapBoolOp::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBitmapBoolOp::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBitmapOr), element_local_name) &&
		0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBitmapAnd), element_local_name))
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name)->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	GPOS_ASSERT(2 == ulSize);

	// add constructed children from child parse handlers
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerOp *pph = dynamic_cast<CParseHandlerOp*>((*this)[ul]);
		AddChildFromParseHandler(pph);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
