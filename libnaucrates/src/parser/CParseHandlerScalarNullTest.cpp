//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerScalarNullTest.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar NullTest.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarNullTest.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarNullTest::CParseHandlerScalarNullTest
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarNullTest::CParseHandlerScalarNullTest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarNullTest::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarNullTest::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& // attrs
	)
{
	if ((0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarIsNull), element_local_name)) ||
		(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarIsNotNull), element_local_name)))
	{

		if( NULL != m_pdxln)
		{
			CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
		}

		BOOL fIsNull = true;

		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarIsNotNull), element_local_name))
		{
			fIsNull = false;
		}

		// parse and create scalar NullTest
		CDXLScalarNullTest *dxl_op = (CDXLScalarNullTest*) CDXLOperatorFactory::PdxlopNullTest(m_parse_handler_mgr->Pmm(), fIsNull);

		// construct node from the created child node
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

		// parse handler for child scalar node
		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarNullTest::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarNullTest::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if ((0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarIsNull), element_local_name)) &&
		(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarIsNotNull), element_local_name)))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(1 == this->Length());


	// add constructed child from child parse handler
	CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp*>((*this)[0]);
	AddChildFromParseHandler(pphChild);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
