//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerLogicalSelect.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		select operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalSelect.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSelect::CParseHandlerLogicalSelect
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalSelect::CParseHandlerLogicalSelect
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSelect::~CParseHandlerLogicalSelect
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalSelect::~CParseHandlerLogicalSelect()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSelect::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalSelect::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& //attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalSelect), element_local_name))
	{
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLLogicalSelect(m_memory_pool));

		// create child node parsers

		// parse handler for logical operator
		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// parse handler for the scalar condition
		CParseHandlerBase *pphOp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOp);

		// store child parse handler in array
		this->Append(pphOp);
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
//		CParseHandlerLogicalSelect::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalSelect::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalSelect), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxln );

	CParseHandlerScalarOp *pphOp = dynamic_cast<CParseHandlerScalarOp*>((*this)[0]);
	CParseHandlerLogicalOp *pphChild = dynamic_cast<CParseHandlerLogicalOp*>((*this)[1]);

	AddChildFromParseHandler(pphOp);
	AddChildFromParseHandler(pphChild);
	
#ifdef GPOS_DEBUG
	m_pdxln->GetOperator()->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
