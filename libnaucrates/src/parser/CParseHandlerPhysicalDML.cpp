//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerPhysicalDML.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing physical DML
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerPhysicalDML.h"

#include "naucrates/dxl/parser/CParseHandlerDirectDispatchInfo.h"
#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerProjList.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalDML::CParseHandlerPhysicalDML
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerPhysicalDML::CParseHandlerPhysicalDML
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerPhysicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_edxldmltype(Edxldmlinsert),
	m_pdrgpul(NULL),
	m_ulAction(0),
	m_ulOid(0),
	m_ctid_colid(0),
	m_segid_colid(0),	
	m_fPreserveOids(false),
	m_ulTupleOidColId(0),
	m_fInputSorted(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalDML::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPhysicalDML::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes &attrs
	)
{
	Edxltoken token_type = EdxltokenPhysicalDMLInsert;
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLInsert), element_local_name) &&
		0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLDelete), element_local_name) &&
		0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLUpdate), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLDelete), element_local_name))
	{
		token_type = EdxltokenPhysicalDMLDelete;
		m_edxldmltype = Edxldmldelete;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLUpdate), element_local_name))
	{
		token_type = EdxltokenPhysicalDMLUpdate;
		m_edxldmltype = Edxldmlupdate;
	}

	const XMLCh *xmlszSourceColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenColumns, token_type);
	m_pdrgpul = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszSourceColIds, EdxltokenColumns, token_type);
	
	m_ulAction = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenActionColId, token_type);
	m_ulOid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenOidColId, token_type);
	m_ctid_colid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCtidColId, token_type);
	m_segid_colid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenGpSegmentIdColId, token_type);

	const XMLCh *xmlszPreserveOids = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenUpdatePreservesOids));
	if (NULL != xmlszPreserveOids)
	{
		m_fPreserveOids = CDXLOperatorFactory::FValueFromXmlstr
											(
											m_parse_handler_mgr->Pmm(),
											xmlszPreserveOids,
											EdxltokenUpdatePreservesOids,
											EdxltokenPhysicalDMLUpdate
											);
	}
	
	if (m_fPreserveOids)
	{
		m_ulTupleOidColId = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTupleOidColId, EdxltokenPhysicalDMLUpdate);
	}

	const XMLCh *xmlszInputSorted = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenInputSorted));
	if (NULL != xmlszInputSorted)
	{
		m_fInputSorted = CDXLOperatorFactory::FValueFromXmlstr
											(
											m_parse_handler_mgr->Pmm(),
											xmlszInputSorted,
											EdxltokenInputSorted,
											EdxltokenPhysicalDMLInsert
											);
	}

	// parse handler for physical operator
	CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphChild);

	//parse handler for the table descriptor
	CParseHandlerBase *pphTabDesc = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTabDesc);

	// parse handler for the proj list
	CParseHandlerBase *pphPrL = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphPrL);

	//parse handler for the direct dispatch info
	CParseHandlerBase *pphDirectDispatch = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphDirectDispatch);
	
	//parse handler for the properties of the operator
	CParseHandlerBase *pphProp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphProp);

	// store child parse handlers in array
	this->Append(pphProp);
	this->Append(pphDirectDispatch);
	this->Append(pphPrL);
	this->Append(pphTabDesc);
	this->Append(pphChild);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalDML::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerPhysicalDML::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EdxlDmlType edxldmltype = CParseHandlerPhysicalDML::EdxlDmlOpType(element_local_name);

	if (EdxldmlSentinel == edxldmltype || m_edxldmltype != edxldmltype)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(5 == this->Length());

	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	GPOS_ASSERT(NULL != pphProp->GetProperties());

	CParseHandlerDirectDispatchInfo *pphDirectDispatch = dynamic_cast<CParseHandlerDirectDispatchInfo *>((*this)[1]);
	GPOS_ASSERT(NULL != pphDirectDispatch->GetDXLDirectDispatchInfo() && NULL != pphDirectDispatch->GetDXLDirectDispatchInfo());

	CParseHandlerProjList *pphPrL = dynamic_cast<CParseHandlerProjList*>((*this)[2]);
	GPOS_ASSERT(NULL != pphPrL->CreateDXLNode());

	CParseHandlerTableDescr *pphTabDesc = dynamic_cast<CParseHandlerTableDescr*>((*this)[3]);
	GPOS_ASSERT(NULL != pphTabDesc->GetTableDescr());
	CDXLTableDescr *pdxltabdesc = pphTabDesc->GetTableDescr();
	pdxltabdesc->AddRef();

	CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[4]);
	GPOS_ASSERT(NULL != pphChild->CreateDXLNode());

	CDXLDirectDispatchInfo *dxl_direct_dispatch_info = pphDirectDispatch->GetDXLDirectDispatchInfo();
	dxl_direct_dispatch_info->AddRef();
	CDXLPhysicalDML *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalDML(m_memory_pool, m_edxldmltype, pdxltabdesc, m_pdrgpul, m_ulAction, m_ulOid, m_ctid_colid, m_segid_colid, m_fPreserveOids, m_ulTupleOidColId, dxl_direct_dispatch_info, m_fInputSorted);
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	
	// set statistics and physical properties
	CParseHandlerUtils::SetProperties(m_dxl_node, pphProp);

	AddChildFromParseHandler(pphPrL);
	AddChildFromParseHandler(pphChild);

#ifdef GPOS_DEBUG
	m_dxl_node->GetOperator()->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalDML::EdxlDmlOpType
//
//	@doc:
//		Parse the dml type from the attribute value
//
//---------------------------------------------------------------------------
EdxlDmlType
CParseHandlerPhysicalDML::EdxlDmlOpType
	(
	const XMLCh *xmlszDmlType
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLInsert), xmlszDmlType))
	{
		return Edxldmlinsert;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLDelete), xmlszDmlType))
	{
		return Edxldmldelete;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLUpdate), xmlszDmlType))
	{
		return Edxldmlupdate;
	}

	return EdxldmlSentinel;
}

// EOF
