//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CParseHandlerMDIndexInfoList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing indexinfo list
//---------------------------------------------------------------------------

#include "naucrates/md/CMDIndexInfo.h"

#include "naucrates/dxl/parser/CParseHandlerMDIndexInfoList.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

// ctor
CParseHandlerMDIndexInfoList::CParseHandlerMDIndexInfoList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdrgpmdIndexInfo(NULL)
{
}

// dtor
CParseHandlerMDIndexInfoList::~CParseHandlerMDIndexInfoList()
{
	CRefCount::SafeRelease(m_pdrgpmdIndexInfo);
}

// returns array of indexinfo
DrgPmdIndexInfo *
CParseHandlerMDIndexInfoList::PdrgpmdIndexInfo()
{
	return m_pdrgpmdIndexInfo;
}

// invoked by Xerces to process an opening tag
void
CParseHandlerMDIndexInfoList::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexInfoList), element_local_name))
	{
		m_pdrgpmdIndexInfo = GPOS_NEW(m_memory_pool) DrgPmdIndexInfo(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexInfo), element_local_name))
	{
		// parse mdid
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenMdid, EdxltokenIndexInfo);

		// parse index partial info
		BOOL fPartial = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenIndexPartial, EdxltokenIndexInfo);

		CMDIndexInfo *pmdIndexInfo = GPOS_NEW(m_memory_pool) CMDIndexInfo
								(
								pmdid,
								fPartial
								);
		m_pdrgpmdIndexInfo->Append(pmdIndexInfo);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

// invoked by Xerces to process a closing tag
void
CParseHandlerMDIndexInfoList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexInfoList), element_local_name))
	{
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndexInfo), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

// EOF
