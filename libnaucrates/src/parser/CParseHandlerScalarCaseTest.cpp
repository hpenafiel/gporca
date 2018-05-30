//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarCaseTest.cpp
//
//	@doc:
//
//		Implementation of the SAX parse handler class for a case test
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarCaseTest.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarCaseTest::CParseHandlerScalarCaseTest
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarCaseTest::CParseHandlerScalarCaseTest
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(pmp, parse_handler_mgr, pphRoot),
	m_pmdidType(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarCaseTest::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarCaseTest::StartElement
	(
	const XMLCh* const, //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, //element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarCaseTest), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// parse type id
	m_pmdidType = CDXLOperatorFactory::PmdidFromAttrs(m_pphm->Pmm(), attrs, EdxltokenTypeId, EdxltokenScalarCaseTest);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarCaseTest::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarCaseTest::EndElement
	(
	const XMLCh* const ,// element_uri
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarCaseTest), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// construct node
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarCaseTest(m_memory_pool, m_pmdidType));

	// deactivate handler
	m_pphm->DeactivateHandler();
}

//EOF
