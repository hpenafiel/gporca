//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarSubqueryQuantified.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for ANY and ALL subquery 
//		operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarSubqueryQuantified.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubqueryQuantified::CParseHandlerScalarSubqueryQuantified
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubqueryQuantified::CParseHandlerScalarSubqueryQuantified
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pdxlop(NULL)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubqueryQuantified::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubqueryQuantified::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	
	GPOS_ASSERT(NULL == m_pdxlop);
		
	// is this a subquery any or subquery all operator
	Edxltoken edxltokenElement = EdxltokenScalarSubqueryAll;
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubqueryAny), element_local_name))
	{
		edxltokenElement = EdxltokenScalarSubqueryAny;
	}
	else if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubqueryAll), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// parse operator id
	IMDId *pmdidOp = CDXLOperatorFactory::PmdidFromAttrs
							(
							m_parse_handler_mgr->Pmm(),
							attrs,
							EdxltokenOpNo,
							edxltokenElement
							);

	// parse operator name
	const XMLCh *xmlszScalarOpName = CDXLOperatorFactory::XmlstrFromAttrs
										(
										attrs,
										EdxltokenOpName,
										edxltokenElement
										);
	
	CWStringDynamic *pstrScalarOpName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlszScalarOpName);
	CMDName *pmdnameScalarOp = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrScalarOpName);
	GPOS_DELETE(pstrScalarOpName);
		
	// parse column id
	ULONG ulColId = CDXLOperatorFactory::UlValueFromAttrs
										(
										m_parse_handler_mgr->Pmm(),
										attrs, 
										EdxltokenColId,
										edxltokenElement
										);
	
	if (EdxltokenScalarSubqueryAny == edxltokenElement)
	{
		m_pdxlop = GPOS_NEW(m_memory_pool) CDXLScalarSubqueryAny(m_memory_pool, pmdidOp, pmdnameScalarOp, ulColId);
	}
	else
	{
		m_pdxlop = GPOS_NEW(m_memory_pool) CDXLScalarSubqueryAll(m_memory_pool, pmdidOp, pmdnameScalarOp, ulColId);
	}
	
	// parse handler for the child nodes
	CParseHandlerBase *pphLgChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphLgChild);

	CParseHandlerBase *pphScChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphScChild);

	// store child parse handler in array
	this->Append(pphScChild);
	this->Append(pphLgChild);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubqueryQuantified::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubqueryQuantified::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubqueryAll), element_local_name) &&
		0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubqueryAny), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node from parsed components
	GPOS_ASSERT(NULL != m_pdxlop);
	GPOS_ASSERT(2 == this->Length());
	
	CParseHandlerScalarOp *pphScChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);
	CParseHandlerLogicalOp *pphLgChild = dynamic_cast<CParseHandlerLogicalOp *>((*this)[1]);
		
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_pdxlop);

	// add constructed child
	AddChildFromParseHandler(pphScChild);
	AddChildFromParseHandler(pphLgChild);

#ifdef GPOS_DEBUG
	m_pdxlop->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG
	
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

