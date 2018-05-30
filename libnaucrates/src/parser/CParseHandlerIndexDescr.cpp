//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerIndexDescr.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the index
//		 descriptor portion of an index scan node
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerIndexDescr.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexDescr::CParseHandlerIndexDescr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerIndexDescr::CParseHandlerIndexDescr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxlid(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexDescr::~CParseHandlerIndexDescr
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerIndexDescr::~CParseHandlerIndexDescr()
{
	CRefCount::SafeRelease(m_pdxlid);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexDescr::Pdxlid
//
//	@doc:
//		Returns the index descriptor constructed by the parse handler
//
//---------------------------------------------------------------------------
CDXLIndexDescr *
CParseHandlerIndexDescr::Pdxlid()
{
	return m_pdxlid;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexDescr::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerIndexDescr::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexDescr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// generate the index descriptor
	m_pdxlid = CDXLOperatorFactory::Pdxlid(m_parse_handler_mgr->Pmm(), attrs);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerIndexDescr::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerIndexDescr::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexDescr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(0 == this->Length());

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
