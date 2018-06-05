//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerXform.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing xform
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerXform.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "gpopt/xforms/CXform.h"
#include "gpopt/xforms/CXformFactory.h"


using namespace gpdxl;
using namespace gpopt;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerXform::CParseHandlerXform
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerXform::CParseHandlerXform
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pxform(NULL)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerXform::~CParseHandlerXform
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerXform::~CParseHandlerXform()
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerXform::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerXform::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenXform), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const XMLCh *xmlstrXformName = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenName, EdxltokenXform);
	CWStringDynamic *pstrXformName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlstrXformName);
	CHAR *szXform = CDXLUtils::CreateMultiByteCharStringFromWCString(m_memory_pool, pstrXformName->GetBuffer());
	m_pxform = CXformFactory::Pxff()->Pxf(szXform);
	GPOS_ASSERT(NULL != m_pxform);

	GPOS_DELETE(pstrXformName);
	GPOS_DELETE_ARRAY(szXform);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerXform::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerXform::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenXform), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}


// EOF

