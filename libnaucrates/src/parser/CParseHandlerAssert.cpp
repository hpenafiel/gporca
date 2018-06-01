//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerAssert.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing physical 
//		assert operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerAssert.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarAssertConstraintList.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerAssert::CParseHandlerAssert
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerAssert::CParseHandlerAssert
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerAssert::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerAssert::StartElement
	(
	const XMLCh* const , // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , // element_qname,
	const Attributes& attrs
	)
{	
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalAssert), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
		
	CHAR *szErrorCode = CDXLOperatorFactory::SzValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenErrorCode, EdxltokenPhysicalAssert);
	if (NULL == szErrorCode || GPOS_SQLSTATE_LENGTH != clib::StrLen(szErrorCode))
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL, 
			gpdxl::ExmiDXLInvalidAttributeValue, 
			CDXLTokens::PstrToken(EdxltokenPhysicalAssert)->GetBuffer(),
			CDXLTokens::PstrToken(EdxltokenErrorCode)->GetBuffer()
			);
	}
	
	m_dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalAssert(m_memory_pool, szErrorCode);

	// ctor created a copy of the error code
	GPOS_DELETE_ARRAY(szErrorCode);
	
	// parse handler for child node
	CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphChild);

	// parse handler for the predicate
	CParseHandlerBase *pphAssertPredicate = CParseHandlerFactory::GetParseHandler
											(
											m_memory_pool, 
											CDXLTokens::XmlstrToken(EdxltokenScalarAssertConstraintList), 
											m_parse_handler_mgr, 
											this
											);
	m_parse_handler_mgr->ActivateParseHandler(pphAssertPredicate);

	// parse handler for the proj list
	CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);

	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);

	this->Append(pphProp);
	this->Append(pphPrL);
	this->Append(pphAssertPredicate);
	this->Append(pphChild);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerAssert::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerAssert::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalAssert), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node from the created child nodes
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerScalarAssertConstraintList *pphAssertPredicate = dynamic_cast<CParseHandlerScalarAssertConstraintList *>((*this)[2]);
	CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[3]);

	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_dxl_op);
	CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

	// add constructed children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphAssertPredicate);
	AddChildFromParseHandler(pphChild);
	
#ifdef GPOS_DEBUG
	m_dxl_op->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
