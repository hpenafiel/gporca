//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerMDGPDBTrigger.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB triggers
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDGPDBTrigger.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#define GPMD_TRIGGER_ROW_BIT 0
#define GPMD_TRIGGER_BEFORE_BIT 1
#define GPMD_TRIGGER_INSERT_BIT 2
#define GPMD_TRIGGER_DELETE_BIT 3
#define GPMD_TRIGGER_UPDATE_BIT 4
#define GPMD_TRIGGER_BITMAP_LEN 5

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBTrigger::CParseHandlerMDGPDBTrigger
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDGPDBTrigger::CParseHandlerMDGPDBTrigger
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname(NULL),
	m_rel_mdid(NULL),
	m_func_mdid(NULL),
	m_iType(0),
	m_fEnabled(false)
{}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBTrigger::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBTrigger::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBTrigger), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	m_mdid = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenGPDBTrigger);

	const XMLCh *xmlszName = CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenName, EdxltokenGPDBTrigger);
	CWStringDynamic *pstrName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszName);
	m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrName);
	GPOS_DELETE(pstrName);
	GPOS_ASSERT(m_mdid->IsValid() && NULL != m_mdname);

	m_rel_mdid = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenRelationMdid, EdxltokenGPDBTrigger);
	m_func_mdid = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenFuncId, EdxltokenGPDBTrigger);

	BOOL rgfProperties[GPMD_TRIGGER_BITMAP_LEN];
	rgfProperties[GPMD_TRIGGER_ROW_BIT] =
			CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGPDBTriggerRow, EdxltokenGPDBTrigger);
	rgfProperties[GPMD_TRIGGER_BEFORE_BIT] =
			CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGPDBTriggerBefore, EdxltokenGPDBTrigger);
	rgfProperties[GPMD_TRIGGER_INSERT_BIT] =
			CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGPDBTriggerInsert, EdxltokenGPDBTrigger);
	rgfProperties[GPMD_TRIGGER_DELETE_BIT] =
			CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGPDBTriggerDelete, EdxltokenGPDBTrigger);
	rgfProperties[GPMD_TRIGGER_UPDATE_BIT] =
			CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGPDBTriggerUpdate, EdxltokenGPDBTrigger);

	for (ULONG ul = 0; ul < GPMD_TRIGGER_BITMAP_LEN; ul++)
	{
		// if the current property flag is true then set the corresponding bit
		if (rgfProperties[ul])
		{
			m_iType |= (1 << ul);
		}
	}

	m_fEnabled = CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGPDBTriggerEnabled, EdxltokenGPDBTrigger);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBTrigger::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBTrigger::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBTrigger), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// construct the MD trigger object
	m_imd_obj = GPOS_NEW(m_memory_pool) CMDTriggerGPDB
								(
								m_memory_pool,
								m_mdid,
								m_mdname,
								m_rel_mdid,
								m_func_mdid,
								m_iType,
								m_fEnabled
								);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
