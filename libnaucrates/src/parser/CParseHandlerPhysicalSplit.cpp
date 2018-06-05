//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerPhysicalSplit.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing physical split
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerPhysicalSplit.h"

#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalSplit::CParseHandlerPhysicalSplit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerPhysicalSplit::CParseHandlerPhysicalSplit
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_deletion_colid_array(NULL),
	m_insert_colid_array(NULL),
	m_ulAction(0),
	m_ctid_colid(0),
	m_segid_colid(0),	
	m_preserve_oids(false),
	m_tuple_oid_col_oid(0)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalSplit::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPhysicalSplit::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalSplit), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const XMLCh *xmlszDeleteColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDeleteCols, EdxltokenPhysicalSplit);
	m_deletion_colid_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDeleteColIds, EdxltokenDeleteCols, EdxltokenPhysicalSplit);

	const XMLCh *xmlszInsertColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenInsertCols, EdxltokenPhysicalSplit);
	m_insert_colid_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszInsertColIds, EdxltokenInsertCols, EdxltokenPhysicalSplit);

	m_ulAction = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenActionColId, EdxltokenPhysicalSplit);
	m_ctid_colid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCtidColId, EdxltokenPhysicalSplit);
	m_segid_colid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenGpSegmentIdColId, EdxltokenPhysicalSplit);

	const XMLCh *xmlszPreserveOids = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenUpdatePreservesOids));
	if (NULL != xmlszPreserveOids)
	{
		m_preserve_oids = CDXLOperatorFactory::FValueFromXmlstr
											(
											m_parse_handler_mgr->Pmm(),
											xmlszPreserveOids,
											EdxltokenUpdatePreservesOids,
											EdxltokenPhysicalSplit
											);
	}	
	if (m_preserve_oids)
	{
		m_tuple_oid_col_oid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTupleOidColId, EdxltokenPhysicalSplit);
	}

	// parse handler for physical operator
	CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);

	// parse handler for the proj list
	CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);

	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);

	// store child parse handlers in array
	this->Append(pphProp);
	this->Append(pphPrL);
	this->Append(child_parse_handler);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalSplit::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPhysicalSplit::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalSplit), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(3 == this->Length());

	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[1]);
	GPOS_ASSERT(NULL != pphPrL->CreateDXLNode());

	CParseHandlerPhysicalOp *child_parse_handler = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[2]);
	GPOS_ASSERT(NULL != child_parse_handler->CreateDXLNode());

	CDXLPhysicalSplit *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalSplit
												(
												m_memory_pool,
												m_deletion_colid_array,
												m_insert_colid_array,
												m_ulAction,
												m_ctid_colid,
												m_segid_colid,
												m_preserve_oids,
												m_tuple_oid_col_oid
												);

	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);

	// set statistics and physical properties
	CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(child_parse_handler);

#ifdef GPOS_DEBUG
	m_dxl_node->GetOperator()->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
