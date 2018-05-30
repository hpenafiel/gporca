//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerLogicalGet.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical get operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalGet.h"

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::CParseHandlerLogicalGet
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalGet::CParseHandlerLogicalGet
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerLogicalOp(pmp, parse_handler_mgr, pphRoot)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::StartElement
//
//	@doc:
//		Start element helper function
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalGet::StartElement
	(
	const XMLCh* const element_local_name,
	Edxltoken edxltoken
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(edxltoken), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// create child node parsers

	// parse handler for table descriptor
	CParseHandlerBase *pphTD = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_pphm, this);
	m_pphm->ActivateParseHandler(pphTD);

	// store child parse handlers in array
	this->Append(pphTD);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalGet::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& //attrs
	)
{
	StartElement(element_local_name, EdxltokenLogicalGet);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::EndElement
//
//	@doc:
//		End element helper function
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalGet::EndElement
	(
	const XMLCh* const element_local_name,
	Edxltoken edxltoken
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(edxltoken), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerTableDescr *pphTD = dynamic_cast<CParseHandlerTableDescr*>((*this)[0]);

	GPOS_ASSERT(NULL != pphTD->Pdxltabdesc());

	CDXLTableDescr *pdxltabdesc = pphTD->Pdxltabdesc();
	pdxltabdesc->AddRef();

	if (EdxltokenLogicalGet == edxltoken)
	{
		m_pdxln = GPOS_NEW(m_pmp) CDXLNode(m_pmp, GPOS_NEW(m_pmp) CDXLLogicalGet(m_pmp, pdxltabdesc));
	}
	else
	{
		GPOS_ASSERT(EdxltokenLogicalExternalGet == edxltoken);
		m_pdxln = GPOS_NEW(m_pmp) CDXLNode(m_pmp, GPOS_NEW(m_pmp) CDXLLogicalExternalGet(m_pmp, pdxltabdesc));
	}

#ifdef GPOS_DEBUG
	m_pdxln->Pdxlop()->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_pphm->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalGet::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EndElement(element_local_name, EdxltokenLogicalGet);
}

// EOF
