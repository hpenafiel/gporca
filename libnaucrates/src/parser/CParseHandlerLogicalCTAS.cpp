//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc
//
//	@filename:
//		CParseHandlerLogicalCTAS.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		CTAS operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalCTAS.h"

#include "naucrates/dxl/parser/CParseHandlerColDescr.h"
#include "naucrates/dxl/parser/CParseHandlerCtasStorageOptions.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalCTAS::CParseHandlerLogicalCTAS
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalCTAS::CParseHandlerLogicalCTAS
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname_schema(NULL),	
	m_mdname(NULL),	
	m_distr_column_pos_array(NULL),
	m_src_colids_array(NULL),
	m_vartypemod_array(NULL),
	m_is_temp_table(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalCTAS::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalCTAS::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalCTAS), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse metadata id
	m_mdid = CDXLOperatorFactory::PmdidFromAttrs
						(
						m_parse_handler_mgr->Pmm(),
						attrs,
						EdxltokenMdid,
						EdxltokenLogicalCTAS
						);
	
	GPOS_ASSERT(IMDId::EmdidGPDBCtas == m_mdid->Emdidt());
	
	// parse table name
	const XMLCh *xmlszTableName = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenName, EdxltokenLogicalCTAS);
	m_mdname = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->Pmm(), xmlszTableName);
	
	const XMLCh *xmlszSchema = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenSchema));
	if (NULL != xmlszSchema)
	{
		m_mdname_schema = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->Pmm(), xmlszSchema);
	}
	
	// parse distribution policy
	const XMLCh *rel_distr_policy_xml = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenRelDistrPolicy, EdxltokenLogicalCTAS);
	m_rel_distr_policy = CDXLOperatorFactory::EreldistrpolicyFromXmlstr(rel_distr_policy_xml);

	if (IMDRelation::EreldistrHash == m_rel_distr_policy)
	{
		// parse distribution columns
		const XMLCh *rel_distr_cols_xml = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDistrColumns, EdxltokenLogicalCTAS);
		m_distr_column_pos_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), rel_distr_cols_xml, EdxltokenDistrColumns, EdxltokenLogicalCTAS);
	}
	
	// parse storage type
	const XMLCh *rel_storage_type_xml = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenRelStorageType, EdxltokenLogicalCTAS);
	m_rel_storage_type = CDXLOperatorFactory::ErelstoragetypeFromXmlstr(rel_storage_type_xml);

	const XMLCh *src_colids_xml = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenInsertCols, EdxltokenLogicalCTAS);
	m_src_colids_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), src_colids_xml, EdxltokenInsertCols, EdxltokenLogicalCTAS);

	const XMLCh *vartypemod_xml = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenVarTypeModList, EdxltokenLogicalCTAS);
	m_vartypemod_array =
			CDXLOperatorFactory::PdrgpiFromXMLCh(m_parse_handler_mgr->Pmm(), vartypemod_xml, EdxltokenVarTypeModList, EdxltokenLogicalCTAS);
	
	m_is_temp_table = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenRelTemporary, EdxltokenLogicalCTAS);
	m_has_oids = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenRelHasOids, EdxltokenLogicalCTAS);

	// create child node parsers

	// parse handler for logical operator
	CParseHandlerBase *child_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(child_parse_handler);

	//parse handler for the storage options
	CParseHandlerBase *ctas_options_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCTASOptions), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(ctas_options_parse_handler);
	
	//parse handler for the column descriptors
	CParseHandlerBase *col_descr_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenColumns), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(col_descr_parse_handler);

	// store child parse handler in array
	this->Append(col_descr_parse_handler);
	this->Append(ctas_options_parse_handler);
	this->Append(child_parse_handler);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalCTAS::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalCTAS::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalCTAS), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(3 == this->Length());

	CParseHandlerColDescr *col_descr_parse_handler = dynamic_cast<CParseHandlerColDescr *>((*this)[0]);
	CParseHandlerCtasStorageOptions *ctas_options_parse_handler = dynamic_cast<CParseHandlerCtasStorageOptions *>((*this)[1]);
	CParseHandlerLogicalOp *child_parse_handler = dynamic_cast<CParseHandlerLogicalOp*>((*this)[2]);

	GPOS_ASSERT(NULL != col_descr_parse_handler->GetColumnDescrDXLArray());
	GPOS_ASSERT(NULL != ctas_options_parse_handler->Pdxlctasopt());
	GPOS_ASSERT(NULL != child_parse_handler->CreateDXLNode());
	
	ColumnDescrDXLArray *pdrgpdxlcd = col_descr_parse_handler->GetColumnDescrDXLArray();
	pdrgpdxlcd->AddRef();
	
	CDXLCtasStorageOptions *pdxlctasopt = ctas_options_parse_handler->Pdxlctasopt();
	pdxlctasopt->AddRef();
	
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode
							(
							m_memory_pool,
							GPOS_NEW(m_memory_pool) CDXLLogicalCTAS
										(
										m_memory_pool, 
										m_mdid, 
										m_mdname_schema, 
										m_mdname, 
										pdrgpdxlcd, 
										pdxlctasopt, 
										m_rel_distr_policy, 
										m_distr_column_pos_array, 
										m_is_temp_table, 
										m_has_oids, 
										m_rel_storage_type, 
										m_src_colids_array,
										m_vartypemod_array
										)
							);
	
	AddChildFromParseHandler(child_parse_handler);

#ifdef GPOS_DEBUG
	m_dxl_node->GetOperator()->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
