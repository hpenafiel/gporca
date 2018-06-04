//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerPhysicalAbstractBitmapScan.h
//
//	@doc:
//		SAX parse handler parent class for parsing bitmap scan operator nodes
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerPhysicalAbstractBitmapScan.h"

#include "naucrates/dxl/operators/CDXLPhysicalAbstractBitmapScan.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalBitmapTableScan.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalDynamicBitmapTableScan.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerScalarBitmapIndexProbe.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalAbstractBitmapScan::StartElementHelper
//
//	@doc:
//		Common StartElement functionality for children of this class
//
//---------------------------------------------------------------------------
void
CParseHandlerPhysicalAbstractBitmapScan::StartElementHelper
	(
 	const XMLCh* const element_local_name,
	Edxltoken token_type
	)
{
	if (0 != XMLString::compareString
				(
				CDXLTokens::XmlstrToken(token_type),
				element_local_name
				))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// create child node parsers in reverse order of their expected occurrence
	// parse handler for table descriptor
	CParseHandlerBase *pphTD = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTD);

	// parse handler for the bitmap access path
	CParseHandlerBase *pphBitmap = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphBitmap);

	// parse handler for the recheck condition
	CParseHandlerBase *pphRecheckCond =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarRecheckCondFilter), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphRecheckCond);

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
	this->Append(pphRecheckCond);
	this->Append(pphBitmap);
	this->Append(pphTD);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalAbstractBitmapScan::EndElementHelper
//
//	@doc:
//		Common EndElement functionality for children of this class
//
//---------------------------------------------------------------------------
void
CParseHandlerPhysicalAbstractBitmapScan::EndElementHelper
	(
	const XMLCh* const element_local_name,
	Edxltoken token_type,
	ULONG ulPartIndexId,
	ULONG ulPartIndexIdPrintable
	)
{
	if (0 != XMLString::compareString
				(
				CDXLTokens::XmlstrToken(token_type),
				element_local_name
				))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct nodes from the created child nodes
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerFilter *pphFilter = dynamic_cast<CParseHandlerFilter *>((*this)[2]);
	CParseHandlerFilter *pphRecheckCond = dynamic_cast<CParseHandlerFilter *>((*this)[3]);
	CParseHandlerScalarOp *pphBitmap =
			dynamic_cast<CParseHandlerScalarOp*>((*this)[4]);
	CParseHandlerTableDescr *pphTD = dynamic_cast<CParseHandlerTableDescr*>((*this)[5]);

	GPOS_ASSERT(NULL != pphTD->GetTableDescr());

	// set table descriptor
	CDXLTableDescr *table_descr = pphTD->GetTableDescr();
	table_descr->AddRef();
	CDXLPhysical *dxl_op = NULL;

	if (EdxltokenPhysicalBitmapTableScan == token_type)
	{
		dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalBitmapTableScan(m_memory_pool, table_descr);
	}
	else
	{
		GPOS_ASSERT(EdxltokenPhysicalDynamicBitmapTableScan == token_type);
		dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalDynamicBitmapTableScan(m_memory_pool, table_descr, ulPartIndexId, ulPartIndexIdPrintable);
	}
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// set statictics and physical properties
	CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

	// add constructed children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);
	AddChildFromParseHandler(pphRecheckCond);

	AddChildFromParseHandler(pphBitmap);

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
