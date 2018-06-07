//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMDRelation.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing relation metadata.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDRelation.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataColumns.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerMDIndexInfoList.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelation::CParseHandlerMDRelation
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDRelation::CParseHandlerMDRelation
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname_schema(NULL),
	m_mdname(NULL),
	m_is_temp_table(false),
	m_has_oids(false),
	m_rel_storage_type(IMDRelation::ErelstorageSentinel),
	m_rel_distr_policy(IMDRelation::EreldistrSentinel),
	m_pdrgpulDistrColumns(NULL),
	m_fConvertHashToRandom(false),
	m_pdrgpulPartColumns(NULL),
	m_pdrgpszPartTypes(NULL),
	m_ulPartitions(0),
	m_pdrgpdrgpulKeys(NULL),
	m_part_constraint(NULL),
	m_level_with_default_part_array(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelation::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelation::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartConstraint), element_local_name))
	{
		GPOS_ASSERT(NULL == m_part_constraint);

		const XMLCh *xmlszDefParts = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDefaultPartition));
		if (NULL != xmlszDefParts)
		{
			m_level_with_default_part_array = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszDefParts, EdxltokenDefaultPartition, EdxltokenRelation);
		}
		else
		{
			// construct an empty keyset
			m_level_with_default_part_array = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
		}
		m_part_constraint_unbounded = CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenPartConstraintUnbounded, EdxltokenRelation);

		CParseHandlerMDIndexInfoList *pphMdlIndexInfo = dynamic_cast<CParseHandlerMDIndexInfoList*>((*this)[1]);
		// relcache translator will send partition constraint expression only when a partitioned relation has indices
		if (pphMdlIndexInfo->PdrgpmdIndexInfo()->Size() > 0)
		{
			// parse handler for part constraints
			CParseHandlerBase *pphPartConstraint= CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
			m_parse_handler_mgr->ActivateParseHandler(pphPartConstraint);
			this->Append(pphPartConstraint);
		}

		return;
	}

	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelation), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// parse main relation attributes: name, id, distribution policy and keys
	ParseRelationAttributes(attrs, EdxltokenRelation);

	// parse whether relation is temporary
	m_is_temp_table = CDXLOperatorFactory::ExtractConvertAttrValueToBool
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
		m_has_oids = CDXLOperatorFactory::ConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszHasOids, EdxltokenRelHasOids, EdxltokenRelation);
	}

	// parse storage type
	const XMLCh *xmlszStorageType = CDXLOperatorFactory::ExtractAttrValue
															(
															attrs,
															EdxltokenRelStorageType,
															EdxltokenRelation
															);
	
	m_rel_storage_type = CDXLOperatorFactory::ParseRelationStorageType(xmlszStorageType);

	const XMLCh *xmlszPartColumns = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenPartKeys));

	if (NULL != xmlszPartColumns)
	{
		m_pdrgpulPartColumns = CDXLOperatorFactory::PdrgpulFromXMLCh
														(
														m_parse_handler_mgr->GetDXLMemoryManager(),
														xmlszPartColumns,
														EdxltokenPartKeys,
														EdxltokenRelation
														);
	}

	const XMLCh *xmlszPartTypes = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenPartTypes));

	if (NULL != xmlszPartTypes)
	{
		m_pdrgpszPartTypes = CDXLOperatorFactory::ExtractConvertPartitionTypeToArray
														(
														m_parse_handler_mgr->GetDXLMemoryManager(),
														xmlszPartTypes,
														EdxltokenPartTypes,
														EdxltokenRelation
														);
	}

	const XMLCh *xmlszPartitions = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenNumLeafPartitions));

	if (NULL != xmlszPartitions)
	{
		m_ulPartitions = CDXLOperatorFactory::ConvertAttrValueToUlong
														(
														m_parse_handler_mgr->GetDXLMemoryManager(),
														xmlszPartitions,
														EdxltokenNumLeafPartitions,
														EdxltokenRelation
														);
	}

	// parse whether a hash distributed relation needs to be considered as random distributed
	const XMLCh *xmlszConvertHashToRandom = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenConvertHashToRandom));
	if (NULL != xmlszConvertHashToRandom)
	{
		m_fConvertHashToRandom = CDXLOperatorFactory::ConvertAttrValueToBool
										(
										m_parse_handler_mgr->GetDXLMemoryManager(), 
										xmlszConvertHashToRandom,
										EdxltokenConvertHashToRandom,
										EdxltokenRelation
										);
	}
	
	// parse children
	ParseChildNodes();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelation::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelation::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	CParseHandlerMDIndexInfoList *pphMdlIndexInfo = dynamic_cast<CParseHandlerMDIndexInfoList*>((*this)[1]);
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartConstraint), element_local_name))
	{
		// relcache translator will send partition constraint expression only when a partitioned relation has indices
		if (pphMdlIndexInfo->PdrgpmdIndexInfo()->Size() > 0)
		{
			CParseHandlerScalarOp *pphPartCnstr = dynamic_cast<CParseHandlerScalarOp *>((*this)[Length() - 1]);
			CDXLNode *pdxlnPartConstraint = pphPartCnstr->CreateDXLNode();
			pdxlnPartConstraint->AddRef();
			m_part_constraint = GPOS_NEW(m_memory_pool) CMDPartConstraintGPDB(m_memory_pool, m_level_with_default_part_array, m_part_constraint_unbounded, pdxlnPartConstraint);
		}
		else
		{
			// no partition constraint expression
			m_part_constraint = GPOS_NEW(m_memory_pool) CMDPartConstraintGPDB(m_memory_pool, m_level_with_default_part_array, m_part_constraint_unbounded, NULL);
		}
		return;
	}

	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelation), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
	
	// construct metadata object from the created child elements
	CParseHandlerMetadataColumns *pphMdCol = dynamic_cast<CParseHandlerMetadataColumns *>((*this)[0]);
	CParseHandlerMetadataIdList *pphMdidlTriggers = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[2]);
	CParseHandlerMetadataIdList *pphMdidlCheckConstraints = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[3]);

	GPOS_ASSERT(NULL != pphMdCol->GetMdColArray());
	GPOS_ASSERT(NULL != pphMdlIndexInfo->PdrgpmdIndexInfo());
	GPOS_ASSERT(NULL != pphMdidlCheckConstraints->GetMdIdArray());

	// refcount child objects
	DrgPmdcol *pdrgpmdcol = pphMdCol->GetMdColArray();
	DrgPmdIndexInfo *pdrgpmdIndexInfos = pphMdlIndexInfo->PdrgpmdIndexInfo();
	DrgPmdid *pdrgpmdidTriggers = pphMdidlTriggers->GetMdIdArray();
	DrgPmdid *pdrgpmdidCheckConstraint = pphMdidlCheckConstraints->GetMdIdArray();
 
	pdrgpmdcol->AddRef();
	pdrgpmdIndexInfos->AddRef();
 	pdrgpmdidTriggers->AddRef();
 	pdrgpmdidCheckConstraint->AddRef();

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDRelationGPDB
								(
									m_memory_pool,
									m_mdid,
									m_mdname,
									m_is_temp_table,
									m_rel_storage_type,
									m_rel_distr_policy,
									pdrgpmdcol,
									m_pdrgpulDistrColumns,
									m_pdrgpulPartColumns,
									m_pdrgpszPartTypes,
									m_ulPartitions,
									m_fConvertHashToRandom,
									m_pdrgpdrgpulKeys,
									pdrgpmdIndexInfos,
									pdrgpmdidTriggers,
									pdrgpmdidCheckConstraint,
									m_part_constraint,
									m_has_oids
								);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelation::ParseRelationAttributes
//
//	@doc:
//		Helper function to parse relation attributes: name, id,
//		distribution policy and keys
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelation::ParseRelationAttributes
	(
	const Attributes& attrs,
	Edxltoken edxltokenElement
	)
{
	// parse table name
	const XMLCh *xmlszTableName = CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenName, edxltokenElement);
	CWStringDynamic *pstrTableName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszTableName);
	m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrTableName);
	GPOS_DELETE(pstrTableName);

	// parse metadata id info
	m_mdid = CDXLOperatorFactory::ExtractConvertAttrValueToMdId(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, edxltokenElement);

	// parse distribution policy
	const XMLCh *rel_distr_policy_xml = CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenRelDistrPolicy, edxltokenElement);
	m_rel_distr_policy = CDXLOperatorFactory::ParseRelationDistPolicy(rel_distr_policy_xml);

	if (m_rel_distr_policy == IMDRelation::EreldistrHash)
	{
		// parse distribution columns
		const XMLCh *rel_distr_cols_xml = CDXLOperatorFactory::ExtractAttrValue(attrs, EdxltokenDistrColumns, edxltokenElement);
		m_pdrgpulDistrColumns = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->GetDXLMemoryManager(), rel_distr_cols_xml, EdxltokenDistrColumns, edxltokenElement);
	}

	// parse keys
	const XMLCh *xmlszKeys = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenKeys));
	if (NULL != xmlszKeys)
	{
		m_pdrgpdrgpulKeys = CDXLOperatorFactory::ExtractConvertUlongTo2DArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszKeys, EdxltokenKeys, edxltokenElement);
	}
	else
	{
		// construct an empty keyset
		m_pdrgpdrgpulKeys = GPOS_NEW(m_memory_pool) ULongPtrArray2D(m_memory_pool);
	}

	m_ulPartitions = CDXLOperatorFactory::ExtractConvertAttrValueToUlong(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenNumLeafPartitions, edxltokenElement,
															true /* optional */, 0 /* default value */);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelation::ParseChildNodes
//
//	@doc:
//		Create and activate the parse handler for the children nodes
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelation::ParseChildNodes()
{
	// parse handler for check constraints
	CParseHandlerBase *pphCheckConstraintList = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenMetadataIdList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphCheckConstraintList);

	// parse handler for trigger list
	CParseHandlerBase *pphTriggerList = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenMetadataIdList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTriggerList);

	// parse handler for index info list
	CParseHandlerBase *pphIndexInfoList = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenIndexInfoList), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphIndexInfoList);

	// parse handler for the columns
	CParseHandlerBase *pphColumns = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenMetadataColumns), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphColumns);

	// store parse handlers
	this->Append(pphColumns);
	this->Append(pphIndexInfoList);
 	this->Append(pphTriggerList);
 	this->Append(pphCheckConstraintList);
}


// EOF
