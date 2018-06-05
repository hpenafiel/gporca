//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CParseHandlerMDRelationCtas.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing CTAS
//		relation metadata.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDRelationCtas.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataColumns.h"
#include "naucrates/dxl/parser/CParseHandlerCtasStorageOptions.h"

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/md/CMDRelationCtasGPDB.h"

#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelationCtas::CParseHandlerMDRelationCtas
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDRelationCtas::CParseHandlerMDRelationCtas
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMDRelation(memory_pool, parse_handler_mgr, parse_handler_root),
	m_vartypemod_array(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelationCtas::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelationCtas::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelationCTAS), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// parse main relation attributes: name, id, distribution policy and keys
	ParseRelationAttributes(attrs, EdxltokenRelation);

	GPOS_ASSERT(IMDId::EmdidGPDBCtas == m_mdid->Emdidt());

	const XMLCh *xmlszSchema = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenSchema));
	if (NULL != xmlszSchema)
	{
		m_mdname_schema = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszSchema);
	}
	
	// parse whether relation is temporary
	m_is_temp_table = CDXLOperatorFactory::FValueFromAttrs
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											attrs,
											EdxltokenRelTemporary,
											EdxltokenRelation
											);

	// parse whether relation has oids
	const XMLCh *xmlszHasOids = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenRelHasOids));
	if (NULL != xmlszHasOids)
	{
		m_has_oids = CDXLOperatorFactory::FValueFromXmlstr(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszHasOids, EdxltokenRelHasOids, EdxltokenRelation);
	}

	// parse storage type
	const XMLCh *xmlszStorageType = CDXLOperatorFactory::XmlstrFromAttrs
															(
															attrs,
															EdxltokenRelStorageType,
															EdxltokenRelation
															);
	m_rel_storage_type = CDXLOperatorFactory::ErelstoragetypeFromXmlstr(xmlszStorageType);

	// parse vartypemod
	const XMLCh *vartypemod_xml = CDXLOperatorFactory::XmlstrFromAttrs
															(
															attrs,
															EdxltokenVarTypeModList,
															EdxltokenRelation
															);
	m_vartypemod_array = CDXLOperatorFactory::PdrgpiFromXMLCh
						(
						m_parse_handler_mgr->GetDXLMemoryManager(),
						vartypemod_xml,
						EdxltokenVarTypeModList,
						EdxltokenRelation
						);

	//parse handler for the storage options
	CParseHandlerBase *ctas_options_parse_handler = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCTASOptions), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(ctas_options_parse_handler);
	
	// parse handler for the columns
	CParseHandlerBase *pphColumns = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenMetadataColumns), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphColumns);
	
	// store parse handlers
	this->Append(pphColumns);
	this->Append(ctas_options_parse_handler);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelationCtas::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelationCtas::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelationCTAS), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerMetadataColumns *pphMdCol = dynamic_cast<CParseHandlerMetadataColumns *>((*this)[0]);
	CParseHandlerCtasStorageOptions *ctas_options_parse_handler = dynamic_cast<CParseHandlerCtasStorageOptions *>((*this)[1]);

	GPOS_ASSERT(NULL != pphMdCol->Pdrgpmdcol());
	GPOS_ASSERT(NULL != ctas_options_parse_handler->Pdxlctasopt());

	DrgPmdcol *pdrgpmdcol = pphMdCol->Pdrgpmdcol();
	CDXLCtasStorageOptions *pdxlctasopt = ctas_options_parse_handler->Pdxlctasopt();

	pdrgpmdcol->AddRef();
	pdxlctasopt->AddRef();

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDRelationCtasGPDB
								(
									m_memory_pool,
									m_mdid,
									m_mdname_schema,
									m_mdname,
									m_is_temp_table,
									m_has_oids,
									m_rel_storage_type,
									m_rel_distr_policy,
									pdrgpmdcol,
									m_pdrgpulDistrColumns,
									m_pdrgpdrgpulKeys,									
									pdxlctasopt,
									m_vartypemod_array
								);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
