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
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdxlprop(NULL),
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
	CRefCount::SafeRelease(m_pdxlprop);
	CRefCount::SafeRelease(m_pdxlstatsderrel);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerProperties::Pdxlprop
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalProperties *
CParseHandlerProperties::Pdxlprop() const
{
	GPOS_ASSERT(NULL != m_pdxlprop);
	return m_pdxlprop;
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
		CParseHandlerBase *pph = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCost), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pph);

		// store parse handler
		this->Append(pph);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsDerivedRelation), element_local_name))
	{
		GPOS_ASSERT(1 == this->Length());

		// create and install derived relation statistics parsers
		CParseHandlerBase *pphStats = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenStatsDerivedRelation), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphStats);

		// store parse handler
		this->Append(pphStats);

		pphStats->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
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
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	GPOS_ASSERT((1 == this->Length()) || (2 == this->Length()));
	
	// assemble the properties container from the cost
	CParseHandlerCost *pph = dynamic_cast<CParseHandlerCost *>((*this)[0]);

	CDXLOperatorCost *pdxlopcost = pph->Pdxlopcost();
	pdxlopcost->AddRef();
	
	if (2 == this->Length())
	{
		CParseHandlerStatsDerivedRelation *pphStats = dynamic_cast<CParseHandlerStatsDerivedRelation *>((*this)[1]);

		CDXLStatsDerivedRelation *pdxlstatsderrel = pphStats->Pdxlstatsderrel();
		pdxlstatsderrel->AddRef();
		m_pdxlstatsderrel = pdxlstatsderrel;
	}

	m_pdxlprop = GPOS_NEW(m_memory_pool) CDXLPhysicalProperties(pdxlopcost);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

