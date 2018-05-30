//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerStatsBound.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing
//	    the bounds of the bucket
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerStatsBound.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsBound::CParseHandlerStatsBound
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerStatsBound::CParseHandlerStatsBound
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxldatum(NULL),
	m_fStatsBoundClosed(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsBound::~CParseHandlerStatsBound
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerStatsBound::~CParseHandlerStatsBound()
{
	m_pdxldatum->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsBound::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerStatsBound::StartElement
	(
	const XMLCh* const,// element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const,// element_qname,
	const Attributes& attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketLowerBound), element_local_name)
	   || 0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketUpperBound), element_local_name))
	{
		GPOS_ASSERT(NULL == m_pdxldatum);

		// translate the datum and add it to the datum array
		CDXLDatum *datum_dxl = CDXLOperatorFactory::Pdxldatum(m_parse_handler_mgr->Pmm(), attrs, EdxltokenDatum);
		m_pdxldatum = datum_dxl;

		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketLowerBound), element_local_name))
		{
			m_fStatsBoundClosed = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenStatsBoundClosed, EdxltokenStatsBucketLowerBound);
		}
		else
		{
			m_fStatsBoundClosed = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenStatsBoundClosed, EdxltokenStatsBucketUpperBound);
		}
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsBound::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerStatsBound::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketLowerBound), element_local_name)
	   && 0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsBucketUpperBound), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxldatum);

	// deactivate handler
  	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
