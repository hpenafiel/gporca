//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerAgg.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing aggregate operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerAgg.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFilter.h"
#include "naucrates/dxl/parser/CParseHandlerGroupingColList.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerAgg::CParseHandlerAgg
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerAgg::CParseHandlerAgg
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerPhysicalOp(pmp, parse_handler_mgr, pphRoot),
	m_pdxlop(NULL)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerAgg::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerAgg::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{	
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalAggregate), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse and create group by operator
	m_pdxlop = (CDXLPhysicalAgg *) CDXLOperatorFactory::PdxlopAgg(m_pphm->Pmm(), attrs);
	
	// create and activate the parse handler for the children nodes in reverse
	// order of their expected appearance
	
	// parse handler for child node
	CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_pphm, this);
	m_pphm->ActivateParseHandler(pphChild);
	
	// parse handler for the filter
	CParseHandlerBase *pphFilter = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarFilter), m_pphm, this);
	m_pphm->ActivateParseHandler(pphFilter);
	
	// parse handler for the proj list
	CParseHandlerBase *pphPrL = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_pphm, this);
	m_pphm->ActivateParseHandler(pphPrL);
	
	//parse handler for the grouping columns list
	CParseHandlerBase *pphGrpColList = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarGroupingColList), m_pphm, this);
	m_pphm->ActivateParseHandler(pphGrpColList);

	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_pphm, this);
	m_pphm->ActivateParseHandler(pphProp);
	
	// store parse handlers
	this->Append(pphProp);
	this->Append(pphGrpColList);
	this->Append(pphPrL);
	this->Append(pphFilter);
	this->Append(pphChild);

}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerAgg::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerAgg::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalAggregate), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// construct node from the created child nodes
	
	GPOS_ASSERT(5 == this->Length());
	
	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerGroupingColList *pphGrpColList = dynamic_cast<CParseHandlerGroupingColList*>((*this)[1]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[2]);
	CParseHandlerFilter *pphFilter = dynamic_cast<CParseHandlerFilter *>((*this)[3]);
	CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[4]);

	// set grouping cols list
	GPOS_ASSERT(NULL != pphGrpColList->PdrgpulGroupingCols());

	ULongPtrArray *pdrgpul = pphGrpColList->PdrgpulGroupingCols();
	pdrgpul->AddRef();
	m_pdxlop->SetGroupingCols(pdrgpul);
	
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, m_pdxlop);	

	// set physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// add children
	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphFilter);
	AddChildFromParseHandler(pphChild);
	
	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
