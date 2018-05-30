//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerColStatsBucket.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing a bucket in
//		a col stats object
//---------------------------------------------------------------------------

#include "naucrates/md/CDXLColStats.h"

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
//		CParseHandlerColStatsBucket::CParseHandlerColStatsBucket
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerColStatsBucket::CParseHandlerColStatsBucket
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_base
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_base),
	m_dFrequency(0.0),
	m_dDistinct(0.0),
	m_pdxldatumLower(NULL),
	m_pdxldatumUpper(NULL),
	m_fLowerClosed(false),
	m_fUpperClosed(false),
	m_pdxlbucket(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStatsBucket::~CParseHandlerColStatsBucket
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerColStatsBucket::~CParseHandlerColStatsBucket()
{
	m_pdxlbucket->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStatsBucket::Pdxlbucket
//
//	@doc:
//		The bucket constructed by the parse handler
//
//---------------------------------------------------------------------------
CDXLBucket *
CParseHandlerColStatsBucket::Pdxlbucket() const
{
	return m_pdxlbucket;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStatsBucket::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerColStatsBucket::StartElement
	(
	const XMLCh* const , // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumnStatsBucket), element_local_name))
	{
		// new column stats bucket

		// parse frequency and distinct values
		m_dFrequency = CDXLOperatorFactory::DValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenStatsFrequency, EdxltokenColumnStatsBucket);
		m_dDistinct = CDXLOperatorFactory::DValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenStatsDistinct, EdxltokenColumnStatsBucket);
		
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketLowerBound), element_local_name))
	{
		// parse lower bound
		m_pdxldatumLower = CDXLOperatorFactory::Pdxldatum(m_pphm->Pmm(), attrs, EdxltokenStatsBucketLowerBound);
		m_fLowerClosed = CDXLOperatorFactory::FValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenStatsBoundClosed, EdxltokenStatsBucketLowerBound);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketUpperBound), element_local_name))
	{
		// parse upper bound
		m_pdxldatumUpper = CDXLOperatorFactory::Pdxldatum(m_pphm->Pmm(), attrs, EdxltokenStatsBucketUpperBound);
		m_fUpperClosed = CDXLOperatorFactory::FValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenStatsBoundClosed, EdxltokenStatsBucketUpperBound);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerColStatsBucket::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerColStatsBucket::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumnStatsBucket), element_local_name))
	{
		m_pdxlbucket = GPOS_NEW(m_memory_pool) CDXLBucket(m_pdxldatumLower, m_pdxldatumUpper, m_fLowerClosed, m_fUpperClosed, m_dFrequency, m_dDistinct);
		
		// deactivate handler
		m_pphm->DeactivateHandler();
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketLowerBound), element_local_name) && 
			0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketUpperBound), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLUnexpectedTag,
			pstr->GetBuffer()
			);
	}	
}

// EOF
