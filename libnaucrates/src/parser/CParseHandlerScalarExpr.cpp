//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerScalarExpr.cpp
//
//	@doc:
//		@see CParseHandlerScalarExpr.h
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarExpr.h"

#include "naucrates/dxl/parser/CParseHandlerOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarExpr::CParseHandlerScalarExpr
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarExpr::CParseHandlerScalarExpr
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(pmp, parse_handler_mgr, pphRoot),
	m_pdxln(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarExpr::~CParseHandlerScalarExpr
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerScalarExpr::~CParseHandlerScalarExpr()
{
	CRefCount::SafeRelease(m_pdxln);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarExpr::Pdxln
//
//	@doc:
//		Root of constructed DXL expression
//
//---------------------------------------------------------------------------
CDXLNode *
CParseHandlerScalarExpr::Pdxln() const
{
	return m_pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarExpr::Edxlphtype
//
//	@doc:
//		Return the type of the parse handler.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerScalarExpr::Edxlphtype() const
{
	return EdxlphScalarExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarExpr::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag for a scalar expression.
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarExpr::StartElement
	(
	const XMLCh* const,
	const XMLCh* const element_local_name,
	const XMLCh* const,
	const Attributes &
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarExpr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL != m_pmp);

	// parse handler for child node
	CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);
	m_pphm->ActivateParseHandler(pphChild);
	Append(pphChild);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarExpr::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag.
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarExpr::EndElement
	(
	const XMLCh* const, //= element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname,
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarExpr), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);
	// extract constructed element
	GPOS_ASSERT(NULL != pphChild && NULL != pphChild->Pdxln());
	m_pdxln = pphChild->Pdxln();
	m_pdxln->AddRef();

	// deactivate handler
	m_pphm->DeactivateHandler();
}
