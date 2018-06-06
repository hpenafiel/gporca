//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerProperties.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing physical properties.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerCost.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerStatsDerivedRelation.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerProperties::CParseHandlerProperties
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerProperties::CParseHandlerProperties
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_dxl_properties(NULL),
	m_pdxlstatsderrel(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerProperties::~CParseHandlerProperties
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerProperties::~CParseHandlerProperties()
{
	CRefCount::SafeRelease(m_dxl_properties);
	CRefCount::SafeRelease(m_pdxlstatsderrel);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerProperties::GetProperties
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties *
CParseHandlerProperties::GetProperties() const
{
	GPOS_ASSERT(NULL != m_dxl_properties);
	return m_dxl_properties;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerProperties::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerProperties::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenProperties), element_local_name))
	{
		// create and install cost and output column parsers
		CParseHandlerBase *pph = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCost), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pph);

		// store parse handler
		this->Append(pph);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsDerivedRelation), element_local_name))
	{
		GPOS_ASSERT(1 == this->Length());

		// create and install derived relation statistics parsers
		CParseHandlerBase *pphStats = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenStatsDerivedRelation), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphStats);

		// store parse handler
		this->Append(pphStats);

		pphStats->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerProperties::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerProperties::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenProperties), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	GPOS_ASSERT((1 == this->Length()) || (2 == this->Length()));
	
	// assemble the properties container from the cost
	CParseHandlerCost *pph = dynamic_cast<CParseHandlerCost *>((*this)[0]);

	CDXLOperatorCost *pdxlopcost = pph->MakeDXLOperatorCost();
	pdxlopcost->AddRef();
	
	if (2 == this->Length())
	{
		CParseHandlerStatsDerivedRelation *pphStats = dynamic_cast<CParseHandlerStatsDerivedRelation *>((*this)[1]);

		CDXLStatsDerivedRelation *pdxlstatsderrel = pphStats->Pdxlstatsderrel();
		pdxlstatsderrel->AddRef();
		m_pdxlstatsderrel = pdxlstatsderrel;
	}

	m_dxl_properties = GPOS_NEW(m_memory_pool) CDXLPhysicalProperties(pdxlopcost);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

