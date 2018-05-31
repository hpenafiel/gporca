//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerMDRequest.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing MD requests
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDRequest.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::CParseHandlerMDRequest
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDRequest::CParseHandlerMDRequest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pdrgpmdid(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::~CParseHandlerMDRequest
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerMDRequest::~CParseHandlerMDRequest()
{
	CRefCount::SafeRelease(m_pdrgpmdid);
	CRefCount::SafeRelease(m_pdrgptr);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRequest::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDRequest), element_local_name))
	{
		// start of MD request section
		GPOS_ASSERT(NULL == m_pdrgpmdid);
		m_pdrgpmdid = GPOS_NEW(m_memory_pool) DrgPmdid(m_memory_pool);
		m_pdrgptr = GPOS_NEW(m_memory_pool) CMDRequest::DrgPtr(m_memory_pool);
		
		return;
	}
	
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMdid), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pdrgpmdid);
		
		// parse mdid
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenValue, EdxltokenMdid);
		m_pdrgpmdid->Append(pmdid);
		
		return;
	}
	
	GPOS_ASSERT(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDTypeRequest), element_local_name));
	GPOS_ASSERT(NULL != m_pdrgptr);

	CSystemId sysid = CDXLOperatorFactory::Sysid(m_parse_handler_mgr->Pmm(), attrs, EdxltokenSysid, EdxltokenMDTypeRequest);

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDTypeRequest), element_local_name))
	{		
		// parse type request
		IMDType::ETypeInfo eti = (IMDType::ETypeInfo) CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeInfo, EdxltokenMDTypeRequest);
		m_pdrgptr->Append(GPOS_NEW(m_memory_pool) CMDRequest::SMDTypeRequest(sysid, eti));
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRequest::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDRequest), element_local_name))
	{
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::Edxlphtype
//
//	@doc:
//		Return the type of the parse handler
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerMDRequest::Edxlphtype() const
{
	return EdxlphMetadataRequest;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::Pdrgpmdid
//
//	@doc:
//		Parsed array of mdids
//
//---------------------------------------------------------------------------
DrgPmdid *
CParseHandlerMDRequest::Pdrgpmdid() const
{
	return m_pdrgpmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRequest::Pdrgptr
//
//	@doc:
//		Parsed array of type requests
//
//---------------------------------------------------------------------------
CMDRequest::DrgPtr *
CParseHandlerMDRequest::Pdrgptr() const
{
	return m_pdrgptr;
}

// EOF
