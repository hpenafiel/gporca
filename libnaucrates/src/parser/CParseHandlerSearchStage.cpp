//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerSearchStage.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing search strategy.
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerSearchStage.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerXform.h"


using namespace gpopt;
using namespace gpdxl;


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStage::CParseHandlerSearchStage
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerSearchStage::CParseHandlerSearchStage
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pxfs(NULL),
	m_costThreshold(GPOPT_INVALID_COST)
{}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStage::~CParseHandlerSearchStage
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerSearchStage::~CParseHandlerSearchStage()
{
	CRefCount::SafeRelease(m_pxfs);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStage::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSearchStage::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStage), element_local_name))
	{
		// start search stage section in the DXL document
		GPOS_ASSERT(NULL == m_pxfs);

		m_pxfs = GPOS_NEW(m_memory_pool) CXformSet(m_memory_pool);

		const XMLCh *xmlszCost =
			CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenCostThreshold, EdxltokenSearchStage);

		m_costThreshold =
			CCost(CDXLOperatorFactory::ConvertAttrValueToDouble(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszCost, EdxltokenCostThreshold, EdxltokenSearchStage));

		const XMLCh *xmlszTime =
			CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenTimeThreshold, EdxltokenSearchStage);

		m_ulTimeThreshold =
			CDXLOperatorFactory::ConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszTime, EdxltokenTimeThreshold, EdxltokenSearchStage);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenXform), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pxfs);

		// start new xform
		CParseHandlerBase *pphXform = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenXform), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphXform);

		// store parse handler
		this->Append(pphXform);

		pphXform->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerSearchStage::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerSearchStage::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{

	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenSearchStage), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	const ULONG size = this->Length();
	// add constructed children from child parse handlers
	for (ULONG ul = 0; ul < size; ul++)
	{
		CParseHandlerXform *pphXform = dynamic_cast<CParseHandlerXform*>((*this)[ul]);
#ifdef GPOS_DEBUG
		BOOL fSet =
#endif // GPOS_DEBUG
			m_pxfs->ExchangeSet(pphXform->Pxform()->Exfid());
		GPOS_ASSERT(!fSet);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();

}

// EOF

