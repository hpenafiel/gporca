//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarSubPlan.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar SubPlan
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarSubPlan.h"
#include "naucrates/dxl/parser/CParseHandlerScalarSubPlanParamList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarSubPlanTestExpr.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLScalarSubPlan.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlan::CParseHandlerScalarSubPlan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlan::CParseHandlerScalarSubPlan
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pmdidFirstCol(NULL),
	m_edxlsubplantype(EdxlSubPlanTypeSentinel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlan::Edxlsubplantype
//
//	@doc:
//		Map character sequence to subplan type
//
//---------------------------------------------------------------------------
EdxlSubPlanType
CParseHandlerScalarSubPlan::Edxlsubplantype
	(
	const XMLCh *xmlszSubplanType
	)
{
	GPOS_ASSERT(NULL != xmlszSubplanType);

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTypeScalar), xmlszSubplanType))
	{
		return EdxlSubPlanTypeScalar;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTypeExists), xmlszSubplanType))
	{
		return EdxlSubPlanTypeExists;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTypeNotExists), xmlszSubplanType))
	{
		return EdxlSubPlanTypeNotExists;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTypeAny), xmlszSubplanType))
	{
		return EdxlSubPlanTypeAny;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTypeAll), xmlszSubplanType))
	{
		return EdxlSubPlanTypeAll;
	}

	// turn Xerces exception in optimizer exception
	GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanType)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlan)->GetBuffer()
		);

	return EdxlSubPlanTypeSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlan::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlan::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlan), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	m_pmdidFirstCol = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenTypeId, EdxltokenScalarSubPlanParam);

	const XMLCh *xmlszSubplanType  = CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenScalarSubPlanType, EdxltokenScalarSubPlan);
	m_edxlsubplantype = Edxlsubplantype(xmlszSubplanType);

	// parse handler for child physical node
	CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);

	// parse handler for params
	CParseHandlerBase *pphParamList = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParamList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphParamList);

	// parse handler for test expression
	CParseHandlerBase *pphTestExpr = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanTestExpr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTestExpr);

	// store parse handlers
	this->Append(pphTestExpr);
	this->Append(pphParamList);
	this->Append(child_parse_handler);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlan::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlan::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlan), element_local_name) && NULL != m_dxl_node)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerScalarSubPlanTestExpr *pphTestExpr = dynamic_cast<CParseHandlerScalarSubPlanTestExpr *>((*this)[0]);
	CParseHandlerScalarSubPlanParamList *pphParamList = dynamic_cast<CParseHandlerScalarSubPlanParamList *>((*this)[1]);
	CParseHandlerPhysicalOp *child_parse_handler = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[2]);

	DrgPdxlcr *pdrgdxlcr = pphParamList->Pdrgdxlcr();
	pdrgdxlcr->AddRef();

	CDXLNode *pdxlnTestExpr = pphTestExpr->PdxlnTestExpr();
	if (NULL != pdxlnTestExpr)
	{
		pdxlnTestExpr->AddRef();
	}
	CDXLScalarSubPlan *dxl_op = (CDXLScalarSubPlan *) CDXLOperatorFactory::MakeDXLSubPlan(m_parse_handler_mgr->GetDXLMemoryManager(), m_pmdidFirstCol, pdrgdxlcr, m_edxlsubplantype, pdxlnTestExpr);

	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// add constructed children
	AddChildFromParseHandler(child_parse_handler);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
