//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerWindowKey.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the window key
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerWindowKey.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerWindowFrame.h"
#include "naucrates/dxl/parser/CParseHandlerSortColList.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowKey::CParseHandlerWindowKey
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerWindowKey::CParseHandlerWindowKey
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxlwk(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowKey::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowKey::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowKey), element_local_name))
	{
		GPOS_ASSERT(NULL == m_pdxlwk);
		m_pdxlwk = GPOS_NEW(m_memory_pool) CDXLWindowKey(m_memory_pool);

		// parse handler for the sorting column list
		CParseHandlerBase *pphSortColList =
				CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSortColList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphSortColList);

		// store parse handler
		this->Append(pphSortColList);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFrame), element_local_name))
	{
		GPOS_ASSERT(1 == this->Length());

		// parse handler for the leading and trailing scalar values
		CParseHandlerBase *pphWf =
				CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenWindowFrame), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphWf);

		// store parse handler
		this->Append(pphWf);
		pphWf->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
			CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowKey::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowKey::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowKey), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL != m_pdxlwk);
	GPOS_ASSERT(1 <= this->Length());

	CParseHandlerSortColList *pphSortColList = dynamic_cast<CParseHandlerSortColList*>((*this)[0]);
	CDXLNode *sort_col_list_dxl = pphSortColList->Pdxln();
	sort_col_list_dxl->AddRef();
	m_pdxlwk->SetSortColList(sort_col_list_dxl);

	if (2 == this->Length())
	{
		CParseHandlerWindowFrame *pphWf = dynamic_cast<CParseHandlerWindowFrame *>((*this)[1]);
		CDXLWindowFrame *pdxlwf = pphWf->GetWindowFrame();
		m_pdxlwk->SetWindowFrame(pdxlwf);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
