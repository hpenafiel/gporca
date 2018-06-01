//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerLogicalJoin.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		join operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalJoin.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalJoin::CParseHandlerLogicalJoin
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalJoin::CParseHandlerLogicalJoin
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
//		CParseHandlerLogicalJoin::~CParseHandlerLogicalJoin
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalJoin::~CParseHandlerLogicalJoin()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalJoin::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalJoin::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalJoin), element_local_name))
	{
		if(NULL == m_pdxln)
		{
			// parse and create logical join operator
			CDXLLogicalJoin *pdxlopJoin = (CDXLLogicalJoin*) CDXLOperatorFactory::PdxlopLogicalJoin(m_parse_handler_mgr->Pmm(), attrs);

			// construct node from the created child nodes
			m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopJoin);
		}
		else
		{
			// This is to support nested join.
			CParseHandlerBase *pphLgJoin = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogicalJoin), m_parse_handler_mgr, this);
			m_parse_handler_mgr->ActivateParseHandler(pphLgJoin);

			// store parse handlers
			this->Append(pphLgJoin);

			pphLgJoin->startElement(element_uri, element_local_name, element_qname, attrs);
		}
	}
	else
	{
		if(NULL == m_pdxln)
		{
			CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
		}

		// The child can either be a CDXLLogicalOp or CDXLScalar
		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, element_local_name, m_parse_handler_mgr, this);

		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handlers
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalJoin::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalJoin::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalJoin), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxln );
	ULONG ulChildrenCount = this->Length();

	// Joins must atleast have 3 children (2 logical operators and 1 scalar operator)
	GPOS_ASSERT(2 < ulChildrenCount);

	// add constructed children from child parse handlers

	// Add the first n-1 logical operator from the first n-1 child parse handler
	for (ULONG ul = 0; ul < ulChildrenCount; ul++)
	{
		CParseHandlerOp *pphOp = dynamic_cast<CParseHandlerOp*>((*this)[ul]);
		GPOS_ASSERT(NULL != pphOp->Pdxln());
		AddChildFromParseHandler(pphOp);
	}

#ifdef GPOS_DEBUG
	m_pdxln->GetOperator()->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
