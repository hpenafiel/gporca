//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerScalarBitmapIndexProbe.cpp
//
//	@doc:
//		SAX parse handler class for parsing bitmap index probe operator nodes
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarBitmapIndexProbe.h"

#include "naucrates/dxl/operators/CDXLScalarBitmapIndexProbe.h"
#include "naucrates/dxl/parser/CParseHandlerIndexCondList.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerIndexDescr.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

using namespace gpdxl;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBitmapIndexProbe::CParseHandlerScalarBitmapIndexProbe
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarBitmapIndexProbe::CParseHandlerScalarBitmapIndexProbe
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
//		CParseHandlerScalarBitmapIndexProbe::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBitmapIndexProbe::StartElement
	(
	const XMLCh* const,  // element_uri
 	const XMLCh* const element_local_name,
	const XMLCh* const,  // element_qname
	const Attributes&  // attrs
	)
{
	if (0 != XMLString::compareString
					(
					CDXLTokens::XmlstrToken(EdxltokenScalarBitmapIndexProbe),
					element_local_name
					))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// create and activate the parse handler for the children nodes in reverse
	// order of their expected appearance

	// parse handler for the index descriptor
	CParseHandlerBase *pphIdxD =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenIndexDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphIdxD);

	// parse handler for the index condition list
	CParseHandlerBase *pphIdxCondList =
			CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarIndexCondList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphIdxCondList);

	// store parse handlers
	this->Append(pphIdxCondList);
	this->Append(pphIdxD);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBitmapIndexProbe::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBitmapIndexProbe::EndElement
	(
	const XMLCh* const,  // element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const  // element_qname
	)
{
	if (0 != XMLString::compareString
				(
				CDXLTokens::XmlstrToken(EdxltokenScalarBitmapIndexProbe),
				element_local_name
				))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node from the created child nodes
	CParseHandlerIndexCondList *pphIdxCondList = dynamic_cast<CParseHandlerIndexCondList *>((*this)[0]);
	CParseHandlerIndexDescr *pphIdxD = dynamic_cast<CParseHandlerIndexDescr *>((*this)[1]);

	CDXLIndexDescr *pdxlid = pphIdxD->Pdxlid();
	pdxlid->AddRef();

	CDXLScalar *dxl_op = GPOS_NEW(m_memory_pool) CDXLScalarBitmapIndexProbe(m_memory_pool, pdxlid);
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// add children
	AddChildFromParseHandler(pphIdxCondList);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
