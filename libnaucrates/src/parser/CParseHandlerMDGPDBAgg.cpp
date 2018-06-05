//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerMDGPDBAgg.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB aggregates.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDGPDBAgg.h"

#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBAgg::CParseHandlerMDGPDBAgg
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMDGPDBAgg::CParseHandlerMDGPDBAgg
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname(NULL),
	m_pmdidTypeResult(NULL),
	m_pmdidTypeIntermediate(NULL),
	m_fOrdered(false),
	m_fSplittable(true),
	m_fHashAggCapable(true)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBAgg::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBAgg::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBAgg), element_local_name))
	{
		// parse agg name
		const XMLCh *xmlszAggName = CDXLOperatorFactory::XmlstrFromAttrs
										(
										attrs,
										EdxltokenName,
										EdxltokenGPDBAgg
										);

		CWStringDynamic *pstrAggName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszAggName);
		
		// create a copy of the string in the CMDName constructor
		m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrAggName);
		
		GPOS_DELETE(pstrAggName);

		// parse metadata id info
		m_mdid = CDXLOperatorFactory::PmdidFromAttrs
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											attrs,
											EdxltokenMdid,
											EdxltokenGPDBAgg
											);
					
		// parse ordered aggregate info
		const XMLCh *xmlszOrderedAgg = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenGPDBIsAggOrdered));
		if (NULL != xmlszOrderedAgg)
		{
			m_fOrdered = CDXLOperatorFactory::FValueFromXmlstr
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												xmlszOrderedAgg,
												EdxltokenGPDBIsAggOrdered,
												EdxltokenGPDBAgg
												);
		}
		
		// parse splittable aggregate info
		const XMLCh *xmlszSplittableAgg = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenGPDBAggSplittable));
		if (NULL != xmlszSplittableAgg)
		{
			m_fSplittable = CDXLOperatorFactory::FValueFromXmlstr
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												xmlszSplittableAgg,
												EdxltokenGPDBAggSplittable,
												EdxltokenGPDBAgg
												);
		}

		// parse hash capable aggragate info
		const XMLCh *xmlszHashAggCapableAgg = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenGPDBAggHashAggCapable));
		if (NULL != xmlszHashAggCapableAgg)
		{
			m_fHashAggCapable = CDXLOperatorFactory::FValueFromXmlstr
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												xmlszHashAggCapableAgg,
												EdxltokenGPDBAggHashAggCapable,
												EdxltokenGPDBAgg
												);
		}
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBAggResultTypeId), element_local_name))
	{
		// parse result type
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidTypeResult = CDXLOperatorFactory::PmdidFromAttrs
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													attrs,
													EdxltokenMdid,
													EdxltokenGPDBAggResultTypeId
													);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBAggIntermediateResultTypeId), element_local_name))
	{
		// parse intermediate result type
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidTypeIntermediate = CDXLOperatorFactory::PmdidFromAttrs
														(
														m_parse_handler_mgr->GetDXLMemoryManager(),
														attrs,
														EdxltokenMdid,
														EdxltokenGPDBAggIntermediateResultTypeId
														);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBAgg::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBAgg::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBAgg), element_local_name))
	{
		// construct the MD agg object from its part
		GPOS_ASSERT(m_mdid->IsValid() && NULL != m_mdname);
		
		m_imd_obj = GPOS_NEW(m_memory_pool) CMDAggregateGPDB(m_memory_pool,
												m_mdid,
												m_mdname,
												m_pmdidTypeResult,
												m_pmdidTypeIntermediate,
												m_fOrdered,
												m_fSplittable,
												m_fHashAggCapable
												);
		
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();

	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBAggResultTypeId), element_local_name) && 
			 0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBAggIntermediateResultTypeId), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

// EOF
