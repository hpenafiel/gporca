//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalDelete.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		delete operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalDelete.h"

#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalDelete::CParseHandlerLogicalDelete
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalDelete::CParseHandlerLogicalDelete
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalDelete::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalDelete::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalDelete), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	m_ctid_colid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCtidColId, EdxltokenLogicalDelete);
	m_segid_colid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenGpSegmentIdColId, EdxltokenLogicalDelete);

	const XMLCh *deletion_colids = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDeleteCols, EdxltokenLogicalDelete);
	m_deletion_colid_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), deletion_colids, EdxltokenDeleteCols, EdxltokenLogicalDelete);

	// parse handler for logical operator
	CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);

	//parse handler for the table descriptor
	CParseHandlerBase *table_descr_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(table_descr_parse_handler);

	// store child parse handler in array
	this->Append(table_descr_parse_handler);
	this->Append(child_parse_handler);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalDelete::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalDelete::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalDelete), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	GPOS_ASSERT(2 == this->Length());

	CParseHandlerTableDescr *table_descr_parse_handler = dynamic_cast<CParseHandlerTableDescr*>((*this)[0]);
	CParseHandlerLogicalOp *logical_op_parse_handler = dynamic_cast<CParseHandlerLogicalOp*>((*this)[1]);

	GPOS_ASSERT(NULL != table_descr_parse_handler->GetTableDescr());
	GPOS_ASSERT(NULL != logical_op_parse_handler->CreateDXLNode());

	CDXLTableDescr *table_descr = table_descr_parse_handler->GetTableDescr();
	table_descr->AddRef();

	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode
							(
							m_memory_pool,
							GPOS_NEW(m_memory_pool) CDXLLogicalDelete(m_memory_pool, table_descr, m_ctid_colid, m_segid_colid, m_deletion_colid_array)
							);

	AddChildFromParseHandler(logical_op_parse_handler);

#ifdef GPOS_DEBUG
	m_dxl_node->GetOperator()->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
