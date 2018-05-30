//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerColStats.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing column
//		statistics.
//---------------------------------------------------------------------------

#include "naucrates/md/CDXLColStats.h"

#include "naucrates/dxl/parser/CParseHandlerColStats.h"
#include "naucrates/dxl/parser/CParseHandlerColStatsBucket.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
using namespace gpmd;
using namespace gpnaucrates;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStats::CParseHandlerColStats
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerColStats::CParseHandlerColStats
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_base
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_base),
	m_mdid(NULL),
	m_md_name(NULL),
	m_width(0.0),
	m_null_freq(0.0),
	m_distinct_remaining(0.0),
	m_freq_remaining(0.0),
	m_is_column_stats_missing(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStats::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerColStats::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumnStats), element_local_name))
	{
		// new column stats object 
		GPOS_ASSERT(NULL == m_mdid);

		// parse mdid and name
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_pphm->Pmm(), attrs, EdxltokenMdid, EdxltokenColumnStats);
		m_mdid = CMDIdColStats::PmdidConvert(pmdid);
		
		// parse column name
		const XMLCh *xmlszColName = CDXLOperatorFactory::XmlstrFromAttrs
																(
																attrs,
																EdxltokenName,
																EdxltokenColumnStats
																);

		CWStringDynamic *pstrColName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlszColName);
		
		// create a copy of the string in the CMDName constructor
		m_md_name = GPOS_NEW(m_pmp) CMDName(m_pmp, pstrColName);
		GPOS_DELETE(pstrColName);
		
		m_width = CDXLOperatorFactory::DValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenWidth, EdxltokenColumnStats);

		const XMLCh *xmlszNullFreq = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColNullFreq));
		if (NULL != xmlszNullFreq)
		{
			m_null_freq = CDXLOperatorFactory::DValueFromXmlstr(m_pphm->Pmm(), xmlszNullFreq, EdxltokenColNullFreq, EdxltokenColumnStats);
		}

		const XMLCh *xmlszDistinctRemain = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColNdvRemain));
		if (NULL != xmlszDistinctRemain)
		{
			m_distinct_remaining = CDXLOperatorFactory::DValueFromXmlstr(m_pphm->Pmm(), xmlszDistinctRemain, EdxltokenColNdvRemain, EdxltokenColumnStats);
		}

		const XMLCh *xmlszFreqRemain = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColFreqRemain));
		if (NULL != xmlszFreqRemain)
		{
			m_freq_remaining = CDXLOperatorFactory::DValueFromXmlstr(m_pphm->Pmm(), xmlszFreqRemain, EdxltokenColFreqRemain, EdxltokenColumnStats);
		}

		const XMLCh *xmlszColStatsMissing = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColStatsMissing));
		if (NULL != xmlszColStatsMissing)
		{
			m_is_column_stats_missing = CDXLOperatorFactory::FValueFromXmlstr(m_pphm->Pmm(), xmlszColStatsMissing, EdxltokenColStatsMissing, EdxltokenColumnStats);
		}

	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumnStatsBucket), element_local_name))
	{
		// new bucket
		CParseHandlerBase *pphStatsBucket = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenColumnStatsBucket), m_pphm, this);
		this->Append(pphStatsBucket);
		
		m_pphm->ActivateParseHandler(pphStatsBucket);	
		pphStatsBucket->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStats::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerColStats::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumnStats), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// get histogram buckets from child parse handlers
	
	DrgPdxlbucket *pdrgpdxlbucket = GPOS_NEW(m_pmp) DrgPdxlbucket(m_pmp);
	
	for (ULONG ul = 0; ul < this->Length(); ul++)
	{
		CParseHandlerColStatsBucket *pphBucket = dynamic_cast<CParseHandlerColStatsBucket *>((*this)[ul]);
				
		CDXLBucket *pdxlbucket = pphBucket->Pdxlbucket();
		pdxlbucket->AddRef();
		
		pdrgpdxlbucket->Append(pdxlbucket);
	}
	
	m_pimdobj = GPOS_NEW(m_pmp) CDXLColStats
							(
							m_pmp,
							m_mdid,
							m_md_name,
							m_width,
							m_null_freq,
							m_distinct_remaining,
							m_freq_remaining,
							pdrgpdxlbucket,
							m_is_column_stats_missing
							);
	
	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
