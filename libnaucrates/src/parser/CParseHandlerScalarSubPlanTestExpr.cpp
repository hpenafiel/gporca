//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CParseHandlerScalarSubPlanTestExpr.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing subplan test expression
//---------------------------------------------------------------------------


#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarSubPlanTestExpr.h"
using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanTestExpr::CParseHandlerScalarSubPlanTestExpr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlanTestExpr::CParseHandlerScalarSubPlanTestExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxlnTestExpr(NULL)
{}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanTestExpr::~CParseHandlerScalarSubPlanTestExpr
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlanTestExpr::~CParseHandlerScalarSubPlanTestExpr()
{
	CRefCount::SafeRelease(m_pdxlnTestExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanTestExpr::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlanTestExpr::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTestExpr), element_local_name))
	{
		// install a scalar element parser for parsing the test expression
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);

		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanTestExpr::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlanTestExpr::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTestExpr), element_local_name) && NULL != m_pdxlnTestExpr)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	if (0 < this->Length())
	{
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);
		if (NULL != pphChild->Pdxln())
		{
			m_pdxlnTestExpr = pphChild->Pdxln();
			m_pdxlnTestExpr->AddRef();
		}
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

