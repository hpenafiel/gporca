//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerIndexScan.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for the index scan operator
//
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerIndexScan.h"

#include "naucrates/dxl/parser/CParseHandlerIndexCondList.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerIndexDescr.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexScan::CParseHandlerIndexScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerIndexScan::CParseHandlerIndexScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_edxlisd(EdxlisdSentinel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexScan::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerIndexScan::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	StartElementHelper(element_local_name, attrs, EdxltokenPhysicalIndexScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexScan::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerIndexScan::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EndElementHelper(element_local_name, EdxltokenPhysicalIndexScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexScan::StartElementHelper
//
//	@doc:
//		Common StartElement functionality for IndexScan and IndexOnlyScan
//
//---------------------------------------------------------------------------
void
CParseHandlerIndexScan::StartElementHelper
	(
	const XMLCh* const element_local_name,
	const Attributes& attrs,
	Edxltoken token_type
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(token_type), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// get the index scan direction from the attribute
	const XMLCh *xmlszIndexScanDirection = CDXLOperatorFactory::XmlstrFromAttrs
																	(
																	attrs,
																	EdxltokenIndexScanDirection,
																	token_type
																	);
	m_edxlisd = CDXLOperatorFactory::EdxljtParseIndexScanDirection
										(
										xmlszIndexScanDirection,
										CDXLTokens::PstrToken(token_type)
										);
	GPOS_ASSERT(EdxlisdSentinel != m_edxlisd);

	// create and activate the parse handler for the children nodes in reverse
	// order of their expected appearance

	CParseHandlerBase *pphTD =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTD);

	// parse handler for the index descriptor
	CParseHandlerBase *pphIdxD =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenIndexDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphIdxD);

	// parse handler for the index condition list
	CParseHandlerBase *pphIdxCondList =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarIndexCondList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphIdxCondList);

	// parse handler for the filter
	CParseHandlerBase *pphFilter =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphFilter);

	// parse handler for the proj list
	CParseHandlerBase *pphPrL =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);

	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);

	// store parse handlers
	this->Append(pphProp);
	this->Append(pphPrL);
	this->Append(pphFilter);
	this->Append(pphIdxCondList);
	this->Append(pphIdxD);
	this->Append(pphTD);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexScan::EndElementHelper
//
//	@doc:
//		Common EndElement functionality for IndexScan and IndexOnlyScan
//
//---------------------------------------------------------------------------
void
CParseHandlerIndexScan::EndElementHelper
	(
	const XMLCh* const element_local_name,
	Edxltoken token_type,
	ULONG ulPartIndexId,
	ULONG ulPartIndexIdPrintable
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
	CParseHandlerIndexCondList *pphIdxCondList = dynamic_cast<CParseHandlerIndexCondList *>((*this)[3]);
	CParseHandlerIndexDescr *pphIdxD = dynamic_cast<CParseHandlerIndexDescr *>((*this)[4]);
	CParseHandlerTableDescr *pphTD = dynamic_cast<CParseHandlerTableDescr *>((*this)[5]);

	CDXLTableDescr *pdxltabdesc = pphTD->Pdxltabdesc();
	pdxltabdesc->AddRef();

	CDXLIndexDescr *pdxlid = pphIdxD->GetIndexDescr();
	pdxlid->AddRef();

	CDXLPhysical *dxl_op = NULL;
	if (EdxltokenPhysicalIndexOnlyScan == token_type)
	{
		dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalIndexOnlyScan(m_memory_pool, pdxltabdesc, pdxlid, m_edxlisd);
		m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	}
	else if (EdxltokenPhysicalIndexScan == token_type)
	{
		dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalIndexScan(m_memory_pool, pdxltabdesc, pdxlid, m_edxlisd);
		m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	}
	else
	{
		GPOS_ASSERT(EdxltokenPhysicalDynamicIndexScan == token_type);

		dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalDynamicIndexScan(m_memory_pool, pdxltabdesc, ulPartIndexId, ulPartIndexIdPrintable, pdxlid, m_edxlisd);
		m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	}

	// set statistics and physical properties
	CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

	// add children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);
	AddChildFromParseHandler(pphIdxCondList);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
