//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerColDescr.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the list of
//		column descriptors in a table descriptor node.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerColDescr.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColDescr::CParseHandlerColDescr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerColDescr::CParseHandlerColDescr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_base
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_base),
	m_current_column_descr(NULL)
{
	m_column_descr_dxl_array = GPOS_NEW(memory_pool) ColumnDescrDXLArray(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColDescr::~CParseHandlerColDescr
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------

CParseHandlerColDescr::~CParseHandlerColDescr()
{
	CRefCount::SafeRelease(m_column_descr_dxl_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColDescr::GetColumnDescrDXLArray
//
//	@doc:
//		Returns the array of column descriptors.
//
//---------------------------------------------------------------------------
ColumnDescrDXLArray *
CParseHandlerColDescr::GetColumnDescrDXLArray()
{
	return m_column_descr_dxl_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColDescr::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerColDescr::StartElement
	(
		const XMLCh* const, // element_uri,
		const XMLCh* const element_local_name,
		const XMLCh* const, // element_qname
		const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenColumns)))
	{
		// start of the columns block
		GPOS_ASSERT(NULL == m_current_column_descr);
	}
	else if (0 == XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenColumn)))
	{
		// start of a new column descriptor
		m_current_column_descr = CDXLOperatorFactory::Pdxlcd(m_pphm->Pmm(), attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColDescr::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerColDescr::EndElement
	(
		const XMLCh* const, // element_uri,
		const XMLCh* const element_local_name,
		const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenColumns)))
	{
		// finish the columns block
		GPOS_ASSERT(NULL != m_column_descr_dxl_array);
		m_pphm->DeactivateHandler();
	}
	else if (0 == XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenColumn)))
	{
		// finish up a column descriptor
		GPOS_ASSERT(NULL != m_current_column_descr);
		GPOS_ASSERT(NULL != m_column_descr_dxl_array);
		m_column_descr_dxl_array->Append(m_current_column_descr);
		// reset column descr
		m_current_column_descr = NULL;
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

// EOF

