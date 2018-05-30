//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerScalarBooleanTest.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar BooleanTest.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarBooleanTest.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBooleanTest::CParseHandlerScalarBooleanTest
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarBooleanTest::CParseHandlerScalarBooleanTest
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, pphRoot),
	m_edxlBooleanTestType(EdxlbooleantestSentinel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBooleanTest::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBooleanTest::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	EdxlBooleanTestType edxlBooleanTestType =
			CParseHandlerScalarBooleanTest::EdxlBooleantestType(element_local_name);

	if (EdxlbooleantestSentinel == edxlBooleanTestType)
	{
		if(NULL == m_pdxln)
		{
			GPOS_RAISE
				(
				gpdxl::ExmaDXL,
				gpdxl::ExmiDXLUnexpectedTag,
				CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name)->GetBuffer()
				);
		}
		else
		{
			CParseHandlerBase *pphChild =
					CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);

			m_pphm->ActivateParseHandler(pphChild);

			// store parse handlers
			this->Append(pphChild);

			pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
		}

		return;
	}

	m_edxlBooleanTestType = edxlBooleanTestType;
	// parse and create scalar BooleanTest
	CDXLScalarBooleanTest *pdxlop =
			(CDXLScalarBooleanTest*) CDXLOperatorFactory::PdxlopBooleanTest(m_pphm->Pmm(), m_edxlBooleanTestType);

	// construct node from the created child nodes
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlop);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBooleanTest::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBooleanTest::EndElement
(
		const XMLCh* const, // element_uri,
		const XMLCh* const element_local_name,
		const XMLCh* const // element_qname
)
{

	EdxlBooleanTestType edxlBooleanTestType = CParseHandlerScalarBooleanTest::EdxlBooleantestType(element_local_name);

	if (EdxlbooleantestSentinel == edxlBooleanTestType )
	{
		GPOS_RAISE
			(
			gpdxl::ExmaDXL,
			gpdxl::ExmiDXLUnexpectedTag,
			CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name)->GetBuffer()
			);
	}
	
	GPOS_ASSERT(edxlBooleanTestType == m_edxlBooleanTestType);
	GPOS_ASSERT(1 == this->Length());

	CParseHandlerScalarOp *pph = dynamic_cast<CParseHandlerScalarOp*>((*this)[0]);
	AddChildFromParseHandler(pph);

	// deactivate handler
	m_pphm->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBooleanTest::EdxlBooleantestType
//
//	@doc:
//		Parse the boolean test type from the attribute value
//
//---------------------------------------------------------------------------
EdxlBooleanTestType
CParseHandlerScalarBooleanTest::EdxlBooleantestType
	(
	const XMLCh *xmlszBooleanTestType
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolTestIsTrue), xmlszBooleanTestType))
	{
		return EdxlbooleantestIsTrue;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolTestIsNotTrue), xmlszBooleanTestType))
	{
		return EdxlbooleantestIsNotTrue;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolTestIsFalse), xmlszBooleanTestType))
	{
		return EdxlbooleantestIsFalse;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolTestIsNotFalse), xmlszBooleanTestType))
	{
		return EdxlbooleantestIsNotFalse;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolTestIsUnknown), xmlszBooleanTestType))
	{
		return EdxlbooleantestIsUnknown;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolTestIsNotUnknown), xmlszBooleanTestType))
	{
		return EdxlbooleantestIsNotUnknown;
	}

	return EdxlbooleantestSentinel;
}

// EOF
