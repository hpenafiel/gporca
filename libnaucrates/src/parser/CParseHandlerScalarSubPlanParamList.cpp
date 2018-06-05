//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarSubPlanParamList.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing the ParamList
//		of a scalar SubPlan
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarSubPlanParamList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarSubPlanParam.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParamList::CParseHandlerScalarSubPlanParamList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlanParamList::CParseHandlerScalarSubPlanParamList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
	m_pdrgdxlcr = GPOS_NEW(memory_pool) DrgPdxlcr(memory_pool);
	m_fParamList = false;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParamList::~CParseHandlerScalarSubPlanParamList
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlanParamList::~CParseHandlerScalarSubPlanParamList()
{
	m_pdrgdxlcr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParamList::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlanParamList::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes &attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParamList), element_local_name))
	{
		// we can't have seen a paramlist already
		GPOS_ASSERT(!m_fParamList);
		// start the paramlist
		m_fParamList = true;
	}
	else if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParam), element_local_name))
	{
		// we must have seen a paramlist already
		GPOS_ASSERT(m_fParamList);

		// start new param
		CParseHandlerBase *pphParam = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParam), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphParam);

		// store parse handler
		this->Append(pphParam);

		pphParam->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParamList::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlanParamList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParamList), element_local_name) && NULL != m_dxl_node)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	// add constructed children from child parse handlers
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerScalarSubPlanParam *pphParam = dynamic_cast<CParseHandlerScalarSubPlanParam *>((*this)[ul]);

		CDXLColRef *pdxlcr = pphParam->Pdxlcr();
		pdxlcr->AddRef();
		m_pdrgdxlcr->Append(pdxlcr);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
