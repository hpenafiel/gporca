//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CParseHandlerCtasStorageOptions.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing CTAS storage
//		options
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerCtasStorageOptions.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCtasStorageOptions::CParseHandlerCtasStorageOptions
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerCtasStorageOptions::CParseHandlerCtasStorageOptions
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pmdnameTablespace(NULL),
	m_pdxlctasopt(NULL),
	m_pdrgpctasopt(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCtasStorageOptions::~CParseHandlerCtasStorageOptions
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerCtasStorageOptions::~CParseHandlerCtasStorageOptions()
{
	CRefCount::SafeRelease(m_pdxlctasopt);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCtasStorageOptions::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCtasStorageOptions::StartElement
	(
	const XMLCh* const , // element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const , // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCTASOptions), element_local_name))
	{
		const XMLCh *xmlszTablespace = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenTablespace));
		if (NULL != xmlszTablespace)
		{
			m_pmdnameTablespace = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszTablespace);
		}
		
		m_ectascommit = CDXLOperatorFactory::EctascommitFromAttr(attrs);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCTASOption), element_local_name))
	{
		// parse option name and value
		ULONG ulType = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenCtasOptionType, EdxltokenCTASOption);
		CWStringBase *pstrName = CDXLOperatorFactory::ExtractConvertAttrValueToStr(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenName, EdxltokenCTASOption);
		CWStringBase *pstrValue = CDXLOperatorFactory::ExtractConvertAttrValueToStr(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenValue, EdxltokenCTASOption);
		BOOL is_null = CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenIsNull, EdxltokenCTASOption);
		
		if (NULL == m_pdrgpctasopt)
		{
			m_pdrgpctasopt = GPOS_NEW(m_memory_pool) CDXLCtasStorageOptions::DrgPctasOpt(m_memory_pool);
		}
		m_pdrgpctasopt->Append(
				GPOS_NEW(m_memory_pool) CDXLCtasStorageOptions::CDXLCtasOption(ulType, pstrName, pstrValue, is_null));
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCtasStorageOptions::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCtasStorageOptions::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCTASOptions), element_local_name))
	{
		m_pdxlctasopt = GPOS_NEW(m_memory_pool) CDXLCtasStorageOptions(m_pmdnameTablespace, m_ectascommit, m_pdrgpctasopt);
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCTASOption), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCtasStorageOptions::Pdxlctasopt
//
//	@doc:
//		Return parsed storage options
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions *
CParseHandlerCtasStorageOptions::Pdxlctasopt() const
{
	return m_pdxlctasopt;
}

// EOF
