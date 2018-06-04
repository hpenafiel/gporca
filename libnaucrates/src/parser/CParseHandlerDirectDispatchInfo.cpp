//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal Inc.
//
//	@filename:
//		CParseHandlerDirectDispatchInfo.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing direct dispatch info
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerDirectDispatchInfo.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"

using namespace gpdxl;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::CParseHandlerDirectDispatchInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerDirectDispatchInfo::CParseHandlerDirectDispatchInfo
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_dxl_datum_array(NULL),
	m_datum_array_combination(NULL),
	m_direct_dispatch_info(NULL)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::~CParseHandlerDirectDispatchInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerDirectDispatchInfo::~CParseHandlerDirectDispatchInfo()
{
	CRefCount::SafeRelease(m_dxl_datum_array);
	CRefCount::SafeRelease(m_direct_dispatch_info);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDirectDispatchInfo::StartElement
	(
	const XMLCh* const , //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), element_local_name))
	{
		m_datum_array_combination = GPOS_NEW(m_memory_pool) DXLDatumArrays(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchKeyValue), element_local_name))
	{
		CRefCount::SafeRelease(m_dxl_datum_array);
		m_dxl_datum_array = GPOS_NEW(m_memory_pool) DXLDatumArray(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDatum), element_local_name))
	{
		GPOS_ASSERT(NULL != m_dxl_datum_array);

		CDXLDatum *datum_dxl = CDXLOperatorFactory::Pdxldatum(m_parse_handler_mgr->Pmm(), attrs, EdxltokenDirectDispatchInfo);
		m_dxl_datum_array->Append(datum_dxl);
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerDirectDispatchInfo::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), element_local_name))
	{
		m_direct_dispatch_info = GPOS_NEW(m_memory_pool) CDXLDirectDispatchInfo(m_datum_array_combination);
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchKeyValue), element_local_name))
	{
		GPOS_ASSERT(NULL != m_dxl_datum_array);
		m_dxl_datum_array->AddRef();
		m_datum_array_combination->Append(m_dxl_datum_array);
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDatum), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerDirectDispatchInfo::GetDXLDirectDispatchInfo
//
//	@doc:
//		Return parsed DXL datum array
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo *
CParseHandlerDirectDispatchInfo::GetDXLDirectDispatchInfo() const
{
	return m_direct_dispatch_info;
}

// EOF

