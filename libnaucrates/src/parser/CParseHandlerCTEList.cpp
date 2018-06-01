//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerCTEList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing CTE lists
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerCTEList.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalCTEProducer.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCTEList::CParseHandlerCTEList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerCTEList::CParseHandlerCTEList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_dxl_array(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCTEList::~CParseHandlerCTEList
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerCTEList::~CParseHandlerCTEList()
{
	CRefCount::SafeRelease(m_dxl_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCTEList::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCTEList::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCTEList), element_local_name))
	{
		GPOS_ASSERT(NULL == m_dxl_array);
		m_dxl_array = GPOS_NEW(m_memory_pool) DXLNodeArray(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalCTEProducer), element_local_name))
	{
		GPOS_ASSERT(NULL != m_dxl_array);

		// start new CTE producer
		CParseHandlerBase *pphCTEProducer = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogicalCTEProducer), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphCTEProducer);
		
		// store parse handler
		this->Append(pphCTEProducer);
		
		pphCTEProducer->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerCTEList::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerCTEList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCTEList), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_dxl_array);

	const ULONG ulLen = this->Length();

	// add CTEs
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CParseHandlerLogicalCTEProducer *pphCTE = dynamic_cast<CParseHandlerLogicalCTEProducer *>((*this)[ul]);
		CDXLNode *pdxlnCTE = pphCTE->CreateDXLNode();
		pdxlnCTE->AddRef();
		m_dxl_array->Append(pdxlnCTE);
	}
		
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
