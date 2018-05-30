//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerWindowKeyList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the list of
//		window keys in the window operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerWindowKey.h"
#include "naucrates/dxl/parser/CParseHandlerWindowKeyList.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowKeyList::CParseHandlerWindowKeyList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerWindowKeyList::CParseHandlerWindowKeyList
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(pmp, parse_handler_mgr, pphRoot),
	m_pdrgpdxlwk(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowKeyList::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowKeyList::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowKeyList), element_local_name))
	{
		m_pdrgpdxlwk = GPOS_NEW(m_pmp) DrgPdxlwk(m_pmp);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowKey), element_local_name))
	{
		// we must have seen a window key list already
		GPOS_ASSERT(NULL != m_pdrgpdxlwk);
		// start new window key element
		CParseHandlerBase *pphWk =
				CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenWindowKey), m_pphm, this);
		m_pphm->ActivateParseHandler(pphWk);

		// store parse handler
		this->Append(pphWk);

		pphWk->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowKeyList::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowKeyList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if ( 0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowKeyList), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL != m_pdrgpdxlwk);

	const ULONG ulSize = this->Length();
	// add the window keys to the list
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerWindowKey *pphWk = dynamic_cast<CParseHandlerWindowKey *>((*this)[ul]);
		m_pdrgpdxlwk->Append(pphWk->Pdxlwk());
	}

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
