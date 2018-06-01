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
	m_pmdnameSchema(NULL),	
	m_mdname(NULL),	
	m_pdrgpulDistr(NULL),
	m_pdrgpulSource(NULL),
	m_pdrgpiVarTypeMod(NULL),
	m_fTemporary(false)
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
		m_pmdnameSchema = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->Pmm(), xmlszSchema);
	}
	
	// parse distribution policy
	const XMLCh *xmlszDistrPolicy = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenRelDistrPolicy, EdxltokenLogicalCTAS);
	m_ereldistrpolicy = CDXLOperatorFactory::EreldistrpolicyFromXmlstr(xmlszDistrPolicy);

	if (IMDRelation::EreldistrHash == m_ereldistrpolicy)
	{
		// parse distribution columns
		const XMLCh *xmlszDistrColumns = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDistrColumns, EdxltokenLogicalCTAS);
		m_pdrgpulDistr = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDistrColumns, EdxltokenDistrColumns, EdxltokenLogicalCTAS);
	}
	
	// parse storage type
	const XMLCh *xmlszStorage = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenRelStorageType, EdxltokenLogicalCTAS);
	m_erelstorage = CDXLOperatorFactory::ErelstoragetypeFromXmlstr(xmlszStorage);

	const XMLCh *xmlszSourceColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenInsertCols, EdxltokenLogicalCTAS);
	m_pdrgpulSource = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszSourceColIds, EdxltokenInsertCols, EdxltokenLogicalCTAS);

	const XMLCh *xmlszVarTypeMod = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenVarTypeModList, EdxltokenLogicalCTAS);
	m_pdrgpiVarTypeMod =
			CDXLOperatorFactory::PdrgpiFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszVarTypeMod, EdxltokenVarTypeModList, EdxltokenLogicalCTAS);
	
	m_fTemporary = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenRelTemporary, EdxltokenLogicalCTAS);
	m_fHasOids = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenRelHasOids, EdxltokenLogicalCTAS);

	// create child node parsers

	// parse handler for logical operator
	CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphChild);

	//parse handler for the storage options
	CParseHandlerBase *pphCTASOptions = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCTASOptions), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphCTASOptions);
	
	//parse handler for the column descriptors
	CParseHandlerBase *pphColDescr = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenColumns), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphColDescr);

	// store child parse handler in array
	this->Append(pphColDescr);
	this->Append(pphCTASOptions);
	this->Append(pphChild);
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

	CParseHandlerColDescr *pphColDescr = dynamic_cast<CParseHandlerColDescr *>((*this)[0]);
	CParseHandlerCtasStorageOptions *pphCTASOptions = dynamic_cast<CParseHandlerCtasStorageOptions *>((*this)[1]);
	CParseHandlerLogicalOp *pphChild = dynamic_cast<CParseHandlerLogicalOp*>((*this)[2]);

	GPOS_ASSERT(NULL != pphColDescr->GetColumnDescrDXLArray());
	GPOS_ASSERT(NULL != pphCTASOptions->Pdxlctasopt());
	GPOS_ASSERT(NULL != pphChild->CreateDXLNode());
	
	ColumnDescrDXLArray *pdrgpdxlcd = pphColDescr->GetColumnDescrDXLArray();
	pdrgpdxlcd->AddRef();
	
	CDXLCtasStorageOptions *pdxlctasopt = pphCTASOptions->Pdxlctasopt();
	pdxlctasopt->AddRef();
	
	m_dxl_node = GPOS_NEW(m_memory_pool) CDXLNode
							(
							m_memory_pool,
							GPOS_NEW(m_memory_pool) CDXLLogicalCTAS
										(
										m_memory_pool, 
										m_mdid, 
										m_pmdnameSchema, 
										m_mdname, 
										pdrgpdxlcd, 
										pdxlctasopt, 
										m_ereldistrpolicy, 
										m_pdrgpulDistr, 
										m_fTemporary, 
										m_fHasOids, 
										m_erelstorage, 
										m_pdrgpulSource,
										m_pdrgpiVarTypeMod
										)
							);
	
	AddChildFromParseHandler(pphChild);

#ifdef GPOS_DEBUG
	m_dxl_node->GetOperator()->AssertValid(m_dxl_node, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
