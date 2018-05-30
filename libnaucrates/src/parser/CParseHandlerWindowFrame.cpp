//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerWindowFrame.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing a window frame
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerWindowFrame.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowFrame::CParseHandlerWindowFrame
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerWindowFrame::CParseHandlerWindowFrame
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(pmp, parse_handler_mgr, pphRoot),
	m_edxlfs(EdxlfsSentinel),
	m_edxlfes(EdxlfesSentinel),
	m_pdxlwf(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowFrame::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowFrame::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFrame), element_local_name))
	{
		m_edxlfs = CDXLOperatorFactory::Edxlfs(attrs);
		m_edxlfes = CDXLOperatorFactory::Edxlfes(attrs);

		// parse handler for the trailing window frame edge
		CParseHandlerBase *pphTrailingVal =
				CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalarWindowFrameTrailingEdge), m_pphm, this);
		m_pphm->ActivateParseHandler(pphTrailingVal);

		// parse handler for the leading scalar values
		CParseHandlerBase *pphLeadingVal =
				CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalarWindowFrameLeadingEdge), m_pphm, this);
		m_pphm->ActivateParseHandler(pphLeadingVal);

		this->Append(pphLeadingVal);
		this->Append(pphTrailingVal);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowFrame::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowFrame::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFrame), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL == m_pdxlwf);
	GPOS_ASSERT(2 == this->Length());

	CParseHandlerScalarOp *pphTrailingVal = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);
	GPOS_ASSERT(NULL != pphTrailingVal);
	CDXLNode *pdxlnTrailing = pphTrailingVal->Pdxln();
	pdxlnTrailing->AddRef();

	CParseHandlerScalarOp *pphLeadingVal = dynamic_cast<CParseHandlerScalarOp *>((*this)[1]);
	GPOS_ASSERT(NULL != pphLeadingVal);
	CDXLNode *pdxlnLeading = pphLeadingVal->Pdxln();
	pdxlnLeading->AddRef();

	m_pdxlwf = GPOS_NEW(m_pmp) CDXLWindowFrame(m_pmp, m_edxlfs, m_edxlfes, pdxlnLeading, pdxlnTrailing);

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
