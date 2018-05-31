//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerPlan.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing physical plans.
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/parser/CParseHandlerDirectDispatchInfo.h"
#include "naucrates/dxl/parser/CParseHandlerPlan.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlan::CParseHandlerPlan
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerPlan::CParseHandlerPlan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_ullId(0),
	m_ullSpaceSize(0),
	m_pdxln(NULL),
	m_pdxlddinfo(NULL)
{}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlan::~CParseHandlerPlan
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerPlan::~CParseHandlerPlan()
{
	CRefCount::SafeRelease(m_pdxln);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlan::Pdxln
//
//	@doc:
//		Root of constructed DXL plan
//
//---------------------------------------------------------------------------
CDXLNode *
CParseHandlerPlan::Pdxln()
{
	return m_pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlan::Edxlphtype
//
//	@doc:
//		Parse handler type
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerPlan::Edxlphtype() const
{
	return EdxlphPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlan::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPlan::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes &attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), element_local_name))
	{
		GPOS_ASSERT(0 < this->Length());
		CParseHandlerBase *pphDirectDispatch = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphDirectDispatch);
		
		// store parse handler
		this->Append(pphDirectDispatch);
		pphDirectDispatch->startElement(element_uri, element_local_name, element_qname, attrs);
		return;
	}
	
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPlan), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse plan id
	const XMLCh *xmlszPlanId = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenPlanId, EdxltokenPlan);
	m_ullId = CDXLOperatorFactory::UllValueFromXmlstr(m_parse_handler_mgr->Pmm(), xmlszPlanId, EdxltokenPlanId, EdxltokenPlan);

	// parse plan space size
	const XMLCh *xmlszPlanSpaceSize = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenPlanSpaceSize, EdxltokenPlan);
	m_ullSpaceSize = CDXLOperatorFactory::UllValueFromXmlstr(m_parse_handler_mgr->Pmm(), xmlszPlanSpaceSize, EdxltokenPlanSpaceSize, EdxltokenPlan);

	// create a parse handler for physical nodes and activate it
	GPOS_ASSERT(NULL != m_memory_pool);
	CParseHandlerBase *pph = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pph);
	
	// store parse handler
	this->Append(pph);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPlan::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPlan::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPlan), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerPhysicalOp *pph = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[0]);
	
	GPOS_ASSERT(NULL != pph->Pdxln());

	// store constructed child
	m_pdxln = pph->Pdxln();
	m_pdxln->AddRef();
	
	if (2 == this->Length())
	{
		CParseHandlerDirectDispatchInfo *pphDirectDispatchInfo = dynamic_cast<CParseHandlerDirectDispatchInfo *>((*this)[1]);
		CDXLDirectDispatchInfo *pdxlddinfo = pphDirectDispatchInfo->Pdxlddinfo();
		GPOS_ASSERT(NULL != pdxlddinfo);
		
		pdxlddinfo->AddRef();
		m_pdxln->SetDirectDispatchInfo(pdxlddinfo);
	}
	// deactivate handler

	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

