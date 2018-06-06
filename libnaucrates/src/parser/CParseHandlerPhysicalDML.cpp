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
	m_dml_type_dxl(Edxldmlinsert),
	m_src_colids_array(NULL),
	m_action_colid(0),
	m_oid_colid(0),
	m_ctid_colid(0),
	m_segid_colid(0),	
	m_preserve_oids(false),
	m_tuple_oid_col_oid(0),
	m_input_sort_req(false)
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
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
	
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLDelete), element_local_name))
	{
		token_type = EdxltokenPhysicalDMLDelete;
		m_dml_type_dxl = Edxldmldelete;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLUpdate), element_local_name))
	{
		token_type = EdxltokenPhysicalDMLUpdate;
		m_dml_type_dxl = Edxldmlupdate;
	}

	const XMLCh *src_colids_xml = CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenColumns, token_type);
	m_src_colids_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->GetDXLMemoryManager(), src_colids_xml, EdxltokenColumns, token_type);
	
	m_action_colid = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenActionColId, token_type);
	m_oid_colid = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenOidColId, token_type);
	m_ctid_colid = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenCtidColId, token_type);
	m_segid_colid = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenGpSegmentIdColId, token_type);

	const XMLCh *preserve_oids_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenUpdatePreservesOids));
	if (NULL != preserve_oids_xml)
	{
		m_preserve_oids = CDXLOperatorFactory::ConvertAttrValueToBool
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											preserve_oids_xml,
											EdxltokenUpdatePreservesOids,
											EdxltokenPhysicalDMLUpdate
											);
	}
	
	if (m_preserve_oids)
	{
		m_tuple_oid_col_oid = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenTupleOidColId, EdxltokenPhysicalDMLUpdate);
	}

	const XMLCh *input_sort_req_xml = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenInputSorted));
	if (NULL != input_sort_req_xml)
	{
		m_input_sort_req = CDXLOperatorFactory::ConvertAttrValueToBool
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											input_sort_req_xml,
											EdxltokenInputSorted,
											EdxltokenPhysicalDMLInsert
											);
	}

	// parse handler for physical operator
	CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);

	//parse handler for the table descriptor
	CParseHandlerBase *table_descr_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(table_descr_parse_handler);

	// parse handler for the proj list
	CParseHandlerBase *proj_list_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(proj_list_parse_handler);

	//parse handler for the direct dispatch info
	CParseHandlerBase *direct_dispatch_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenDirectDispatchInfo), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(direct_dispatch_parse_handler);
	
	//parse handler for the properties of the operator
	CParseHandlerBase *prop_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenProperties), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(prop_parse_handler);

	// store child parse handlers in array
	this->Append(prop_parse_handler);
	this->Append(direct_dispatch_parse_handler);
	this->Append(proj_list_parse_handler);
	this->Append(table_descr_parse_handler);
	this->Append(child_parse_handler);
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
	EdxlDmlType dml_type_dxl = CParseHandlerPhysicalDML::GetDmlOpType(element_local_name);

	if (EdxldmlSentinel == dml_type_dxl || m_dml_type_dxl != dml_type_dxl)
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	GPOS_ASSERT(5 == this->Length());

	CParseHandlerProperties *prop_parse_handler = dynamic_cast<CParseHandlerProperties *>((*this)[0]);
	GPOS_ASSERT(NULL != prop_parse_handler->GetProperties());

	CParseHandlerDirectDispatchInfo *direct_dispatch_parse_handler = dynamic_cast<CParseHandlerDirectDispatchInfo *>((*this)[1]);
	GPOS_ASSERT(NULL != direct_dispatch_parse_handler->GetDXLDirectDispatchInfo() && NULL != direct_dispatch_parse_handler->GetDXLDirectDispatchInfo());

	CParseHandlerProjList *proj_list_parse_handler = dynamic_cast<CParseHandlerProjList*>((*this)[2]);
	GPOS_ASSERT(NULL != proj_list_parse_handler->CreateDXLNode());

	CParseHandlerTableDescr *table_descr_parse_handler = dynamic_cast<CParseHandlerTableDescr*>((*this)[3]);
	GPOS_ASSERT(NULL != table_descr_parse_handler->MakeDXLTableDescr());
	CDXLTableDescr *table_descr = table_descr_parse_handler->MakeDXLTableDescr();
	table_descr->AddRef();

	CParseHandlerPhysicalOp *child_parse_handler = dynamic_cast<CParseHandlerPhysicalOp*>((*this)[4]);
	GPOS_ASSERT(NULL != child_parse_handler->CreateDXLNode());

	CDXLDirectDispatchInfo *dxl_direct_dispatch_info = direct_dispatch_parse_handler->GetDXLDirectDispatchInfo();
	dxl_direct_dispatch_info->AddRef();
	CDXLPhysicalDML *dxl_op = GPOS_NEW(m_memory_pool) CDXLPhysicalDML(m_memory_pool, m_dml_type_dxl, table_descr, m_src_colids_array, m_action_colid, m_oid_colid, m_ctid_colid, m_segid_colid, m_preserve_oids, m_tuple_oid_col_oid, dxl_direct_dispatch_info, m_input_sort_req);
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	
	// set statistics and physical properties
	CParseHandlerUtils::SetProperties(m_dxl_node, prop_parse_handler);

	AddChildFromParseHandler(proj_list_parse_handler);
	AddChildFromParseHandler(child_parse_handler);

#ifdef GPOS_DEBUG
	m_dxl_node->GetOperator()->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPhysicalDML::GetDmlOpType
//
//	@doc:
//		Parse the dml type from the attribute value
//
//---------------------------------------------------------------------------
EdxlDmlType
CParseHandlerPhysicalDML::GetDmlOpType
	(
	const XMLCh *dml_op_type_xml
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLInsert), dml_op_type_xml))
	{
		return Edxldmlinsert;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLDelete), dml_op_type_xml))
	{
		return Edxldmldelete;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalDMLUpdate), dml_op_type_xml))
	{
		return Edxldmlupdate;
	}

	return EdxldmlSentinel;
}

// EOF
