//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalGroupBy.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		GroupBy operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalGroupBy.h"

#include "naucrates/dxl/parser/CParseHandlerGroupingColList.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGroupBy::CParseHandlerLogicalGroupBy
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalGroupBy::CParseHandlerLogicalGroupBy
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, pphRoot)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGroupBy::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalGroupBy::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& //attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalGrpBy), element_local_name))
	{
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLLogicalGroupBy(m_memory_pool));

		// create child node parsers

		// parse handler for logical operator
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// parse handler for the proj list
		CParseHandlerBase *pphPrL = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphPrL);

		//parse handler for the grouping columns list
		CParseHandlerBase *pphGrpCollList = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarGroupingColList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphGrpCollList);

		// store child parse handler in array
		this->Append(pphGrpCollList);
		this->Append(pphPrL);
		this->Append(pphChild);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGroupBy::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalGroupBy::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalGrpBy), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxln );
	GPOS_ASSERT(3 == this->Length());

	CParseHandlerGroupingColList *pphGrpCollList = dynamic_cast<CParseHandlerGroupingColList*>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	CParseHandlerLogicalOp *pphChild = dynamic_cast<CParseHandlerLogicalOp*>((*this)[2]);

	GPOS_ASSERT(NULL != pphPrL->Pdxln());
	GPOS_ASSERT(NULL != pphChild->Pdxln());

	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphChild);

	CDXLLogicalGroupBy *pdxlopGrpby = static_cast<CDXLLogicalGroupBy*>(m_pdxln->Pdxlop());

	// set grouping cols list
	GPOS_ASSERT(NULL != pphGrpCollList->PdrgpulGroupingCols());

	ULongPtrArray *pdrgpul = pphGrpCollList->PdrgpulGroupingCols();
	pdrgpul->AddRef();
	pdxlopGrpby->SetGroupingColumns(pdrgpul);


#ifdef GPOS_DEBUG
	pdxlopGrpby->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
