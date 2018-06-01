//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerTableScan.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing table scan operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerTableScan.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTableScan::CParseHandlerTableScan
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerTableScan::CParseHandlerTableScan
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
//		CParseHandlerTableScan::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerTableScan::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& // attrs
	)
{
	StartElement(element_local_name, EdxltokenPhysicalTableScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTableScan::StartElement
//
//	@doc:
//		Start element helper function
//
//---------------------------------------------------------------------------
void
CParseHandlerTableScan::StartElement
	(
	const XMLCh* const element_local_name,
	Edxltoken token_type
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(token_type), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	if (EdxltokenPhysicalTableScan == token_type)
	{
		m_dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalTableScan(m_memory_pool);
	}
	else
	{
		GPOS_ASSERT(EdxltokenPhysicalExternalScan == token_type);
		m_dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalExternalScan(m_memory_pool);
	}

	// create child node parsers in reverse order of their expected occurrence

	// parse handler for table descriptor
	CParseHandlerBase *pphTD = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTD);

	// parse handler for the filter
	CParseHandlerBase *pphFilter = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphFilter);

	// parse handler for the proj list
	CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);

	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);

	// store child parse handlers in array
	this->Append(pphProp);
	this->Append(pphPrL);
	this->Append(pphFilter);
	this->Append(pphTD);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTableScan::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerTableScan::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EndElement(element_local_name, EdxltokenPhysicalTableScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTableScan::EndElement
//
//	@doc:
//		End element helper function
//
//---------------------------------------------------------------------------
void
CParseHandlerTableScan::EndElement
	(
	const XMLCh* const element_local_name,
	Edxltoken token_type
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(token_type), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node from the created child nodes
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerFilter *pphFilter = dynamic_cast<CParseHandlerFilter *>((*this)[2]);
	CParseHandlerTableDescr *pphTD = dynamic_cast<CParseHandlerTableDescr*>((*this)[3]);

	GPOS_ASSERT(NULL != pphTD->Pdxltabdesc());

	// set table descriptor
	CDXLTableDescr *pdxltabdesc = pphTD->Pdxltabdesc();
	pdxltabdesc->AddRef();
	m_dxl_op->SetTableDescriptor(pdxltabdesc);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_dxl_op);
	// set statictics and physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// add constructed children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);

#ifdef GPOS_DEBUG
	m_dxl_op->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

