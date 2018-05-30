//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerBase.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the list of
//		column descriptors in a table descriptor node.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerBase.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/xml/CDXLMemoryManager.h"


using namespace gpdxl;
using namespace gpos;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::CParseHandlerBase
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerBase::CParseHandlerBase
	(
	IMemoryPool *pmp, 
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	m_memory_pool(pmp),
	m_pphm(parse_handler_mgr),
	m_pphRoot(pphRoot)
{
	GPOS_ASSERT(NULL != pmp);
	GPOS_ASSERT(NULL != parse_handler_mgr);
	
	m_pdrgpph = GPOS_NEW(m_memory_pool) DrgPph(m_memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::~CParseHandlerBase
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------

CParseHandlerBase::~CParseHandlerBase()
{
	m_pdrgpph->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::Edxlphtype
//
//	@doc:
//		Return the type of the parse handler. Currently we overload this method to 
//		return a specific type for the plan, query, metadata and traceflags parse handlers.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerBase::Edxlphtype() const
{
	return EdxlphOther;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::ReplaceParseHandler
//
//	@doc:
//		Replaces a parse handler in the parse handler array with a new one
//
//---------------------------------------------------------------------------
void
CParseHandlerBase::ReplaceParseHandler
	(
	CParseHandlerBase *pphOld,
	CParseHandlerBase *pphNew
	)
{
	ULONG ulPos = 0;
	
	GPOS_ASSERT(NULL != m_pdrgpph);
	
	for (ulPos = 0; ulPos < m_pdrgpph->Size(); ulPos++)
	{
		if ((*m_pdrgpph)[ulPos] == pphOld)
		{
			break;
		}
	}
	
	// assert old parse handler was found in array
	GPOS_ASSERT(ulPos < m_pdrgpph->Size());
	
	m_pdrgpph->Replace(ulPos, pphNew);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::startElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerBase::startElement
	(
		const XMLCh* const element_uri,
		const XMLCh* const element_local_name,
		const XMLCh* const element_qname,
		const Attributes& attrs
	)
{
	StartElement(element_uri, element_local_name, element_qname, attrs);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::endElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerBase::endElement
	(
		const XMLCh* const element_uri,
		const XMLCh* const element_local_name,
		const XMLCh* const element_qname
	)
{
	EndElement(element_uri, element_local_name, element_qname);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerBase::error
//
//	@doc:
//		Invoked by Xerces to process an error
//
//---------------------------------------------------------------------------
void
CParseHandlerBase::error
	(
	const SAXParseException& toCatch
	)
{
	CHAR* szMessage = XMLString::transcode(toCatch.getMessage(), m_pphm->Pmm());
	GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLValidationError, szMessage);
}

// EOF

