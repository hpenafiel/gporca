//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerCost.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing cost estimates.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerCost.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCost::CParseHandlerCost
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerCost::CParseHandlerCost
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxlopcost(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCost::~CParseHandlerCost
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerCost::~CParseHandlerCost()
{
	CRefCount::SafeRelease(m_pdxlopcost);	
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCost::Pdxlopcost
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLOperatorCost *
CParseHandlerCost::Pdxlopcost()
{
	return m_pdxlopcost;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCost::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCost::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCost), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// get cost estimates from attributes
	m_pdxlopcost = CDXLOperatorFactory::Pdxlopcost(m_parse_handler_mgr->Pmm(), attrs);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCost::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCost::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCost), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
