//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerScalarOpExpr.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar OpExpr.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarOpExpr.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpExpr::CParseHandlerScalarOpExpr
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarOpExpr::CParseHandlerScalarOpExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(pmp, parse_handler_mgr, pphRoot),
	m_ulChildCount(0)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpExpr::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarOpExpr::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarOpExpr), element_local_name) && (NULL == m_pdxln))
	{
		// parse and create scalar OpExpr
		CDXLScalarOpExpr *pdxlop = (CDXLScalarOpExpr*) CDXLOperatorFactory::PdxlopOpExpr(m_pphm->Pmm(), attrs);

		// construct node from the created child nodes
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlop);
	}
	else if (NULL != m_pdxln)
	{
		if (2 > m_ulChildCount)
		{
			CParseHandlerBase *pphOp = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);

			m_pphm->ActivateParseHandler(pphOp);

			// store parse handlers
			this->Append(pphOp);

			pphOp->startElement(element_uri, element_local_name, element_qname, attrs);

			m_ulChildCount++;
		}
		else
		{
			CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());		}
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);

		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLIncorrectNumberOfChildren, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpExpr::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarOpExpr::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarOpExpr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	GPOS_ASSERT(1 == ulSize || 2 == ulSize);

	// add constructed children from child parse handlers
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerScalarOp *pphOp = dynamic_cast<CParseHandlerScalarOp*>((*this)[ul]);
		AddChildFromParseHandler(pphOp);
	}

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
