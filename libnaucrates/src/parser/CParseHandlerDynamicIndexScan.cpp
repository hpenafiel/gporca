//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerDynamicIndexScan.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for dynamic index scan operators
//
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerDynamicIndexScan.h"

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
//		CParseHandlerDynamicIndexScan::CParseHandlerDynamicIndexScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerDynamicIndexScan::CParseHandlerDynamicIndexScan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerIndexScan(memory_pool, parse_handler_mgr, parse_handler_root),
	m_ulPartIndexId(0),
	m_ulPartIndexIdPrintable(0)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDynamicIndexScan::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDynamicIndexScan::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	StartElementHelper(element_local_name, attrs, EdxltokenPhysicalDynamicIndexScan);
	
	m_ulPartIndexId = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPartIndexId, EdxltokenPhysicalDynamicIndexScan);
	m_ulPartIndexIdPrintable = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPartIndexIdPrintable, EdxltokenPhysicalDynamicIndexScan, true /*fOptional*/, m_ulPartIndexId);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDynamicIndexScan::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDynamicIndexScan::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EndElementHelper(element_local_name, EdxltokenPhysicalDynamicIndexScan, m_ulPartIndexId, m_ulPartIndexIdPrintable);
}


// EOF
