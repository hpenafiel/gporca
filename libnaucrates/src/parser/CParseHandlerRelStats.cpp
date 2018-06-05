//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerRelStats.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing base relation
//		statistics.
//---------------------------------------------------------------------------

#include "naucrates/md/CDXLRelStats.h"

#include "naucrates/dxl/parser/CParseHandlerRelStats.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpnaucrates;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerRelStats::CParseHandlerRelStats
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerRelStats::CParseHandlerRelStats
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerRelStats::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerRelStats::StartElement
	(
	const XMLCh* const , // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , // element_qname,
	const Attributes& attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelationStats), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse table name
	const XMLCh *xmlszTableName = CDXLOperatorFactory::XmlstrFromAttrs
															(
															attrs,
															EdxltokenName,
															EdxltokenRelationStats
															);

	CWStringDynamic *pstrTableName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszTableName);
	
	// create a copy of the string in the CMDName constructor
	CMDName *mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrTableName);
	
	GPOS_DELETE(pstrTableName);
	

	// parse metadata id info
	IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenRelationStats);
	
	// parse rows

	CDouble dRows = CDXLOperatorFactory::DValueFromAttrs
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											attrs,
											EdxltokenRows,
											EdxltokenRelationStats
											);
	
	BOOL fEmpty = false;
	const XMLCh *xmlszEmpty = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenEmptyRelation));
	if (NULL != xmlszEmpty)
	{
		fEmpty = CDXLOperatorFactory::FValueFromXmlstr
										(
										m_parse_handler_mgr->GetDXLMemoryManager(),
										xmlszEmpty,
										EdxltokenEmptyRelation,
										EdxltokenStatsDerivedRelation
										);
	}

	m_imd_obj = GPOS_NEW(m_memory_pool) CDXLRelStats(m_memory_pool, CMDIdRelStats::PmdidConvert(pmdid), mdname, dRows, fEmpty);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerRelStats::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerRelStats::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelationStats), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
