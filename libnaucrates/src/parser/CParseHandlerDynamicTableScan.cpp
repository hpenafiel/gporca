//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerDynamicTableScan.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing dynamic 
//		table scan operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerDynamicTableScan.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLPhysicalDynamicTableScan.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDynamicTableScan::CParseHandlerDynamicTableScan
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerDynamicTableScan::CParseHandlerDynamicTableScan
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
//		CParseHandlerDynamicTableScan::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDynamicTableScan::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDynamicTableScan), 
									element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	m_ulPartIndexId = CDXLOperatorFactory::UlValueFromAttrs
											(
											m_parse_handler_mgr->Pmm(), 
											attrs, 
											EdxltokenPartIndexId, 
											EdxltokenPhysicalDynamicTableScan
											);

	m_ulPartIndexIdPrintable = CDXLOperatorFactory::UlValueFromAttrs
											(
											m_parse_handler_mgr->Pmm(),
											attrs,
											EdxltokenPartIndexIdPrintable,
											EdxltokenPhysicalDynamicTableScan,
											true, //fOptional
											m_ulPartIndexId
											);

	// create child node parsers in reverse order of their expected occurrence

	// parse handler for table descriptor
	CParseHandlerBase *pphTD = 
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTD);

	// parse handler for the filter
	CParseHandlerBase *pphFilter = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphFilter);

	// parse handler for the proj list
	CParseHandlerBase *pphPrL = 
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);
	
	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = 
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);
	
	// store child parse handlers in array
	this->Append(pphProp);
	this->Append(pphPrL);
	this->Append(pphFilter);
	this->Append(pphTD);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDynamicTableScan::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDynamicTableScan::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDynamicTableScan), 
									element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node from the created child nodes
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerFilter *pphFilter = dynamic_cast<CParseHandlerFilter*>((*this)[2]);
	CParseHandlerTableDescr *pphTD = dynamic_cast<CParseHandlerTableDescr*>((*this)[3]);


	// set table descriptor
	CDXLTableDescr *pdxltabdesc = pphTD->Pdxltabdesc();
	pdxltabdesc->AddRef();
	CDXLPhysicalDynamicTableScan *dxl_op = 
			GPOS_NEW(m_memory_pool) CDXLPhysicalDynamicTableScan(m_memory_pool, pdxltabdesc, m_ulPartIndexId, m_ulPartIndexIdPrintable);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);	
	// set statictics and physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// add constructed children	
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG
	
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

