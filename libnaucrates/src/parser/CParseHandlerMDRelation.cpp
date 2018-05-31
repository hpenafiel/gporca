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
	m_pmdnameSchema(NULL),
	m_mdname(NULL),
	m_fTemporary(false),
	m_fHasOids(false),
	m_erelstorage(IMDRelation::ErelstorageSentinel),
	m_ereldistrpolicy(IMDRelation::EreldistrSentinel),
	m_pdrgpulDistrColumns(NULL),
	m_fConvertHashToRandom(false),
	m_pdrgpulPartColumns(NULL),
	m_pdrgpszPartTypes(NULL),
	m_ulPartitions(0),
	m_pdrgpdrgpulKeys(NULL),
	m_ppartcnstr(NULL),
	m_pdrgpulDefaultParts(NULL)
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
		GPOS_ASSERT(NULL == m_ppartcnstr);

		const XMLCh *xmlszDefParts = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDefaultPartition));
		if (NULL != xmlszDefParts)
		{
			m_pdrgpulDefaultParts = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDefParts, EdxltokenDefaultPartition, EdxltokenRelation);
		}
		else
		{
			// construct an empty keyset
			m_pdrgpulDefaultParts = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
		}
		m_fPartConstraintUnbounded = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPartConstraintUnbounded, EdxltokenRelation);

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
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// parse main relation attributes: name, id, distribution policy and keys
	ParseRelationAttributes(attrs, EdxltokenRelation);

	// parse whether relation is temporary
	m_fTemporary = CDXLOperatorFactory::FValueFromAttrs
											(
											m_parse_handler_mgr->Pmm(),
											attrs,
											EdxltokenRelTemporary,
											EdxltokenRelation
											);

	// parse whether relation has oids
	const XMLCh *xmlszHasOids = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenRelHasOids));
	if (NULL != xmlszHasOids)
	{
		m_fHasOids = CDXLOperatorFactory::FValueFromXmlstr(m_parse_handler_mgr->Pmm(), xmlszHasOids, EdxltokenRelHasOids, EdxltokenRelation);
	}

	// parse storage type
	const XMLCh *xmlszStorageType = CDXLOperatorFactory::XmlstrFromAttrs
															(
															attrs,
															EdxltokenRelStorageType,
															EdxltokenRelation
															);
	
	m_erelstorage = CDXLOperatorFactory::ErelstoragetypeFromXmlstr(xmlszStorageType);

	const XMLCh *xmlszPartColumns = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenPartKeys));

	if (NULL != xmlszPartColumns)
	{
		m_pdrgpulPartColumns = CDXLOperatorFactory::PdrgpulFromXMLCh
														(
														m_parse_handler_mgr->Pmm(),
														xmlszPartColumns,
														EdxltokenPartKeys,
														EdxltokenRelation
														);
	}

	const XMLCh *xmlszPartTypes = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenPartTypes));

	if (NULL != xmlszPartTypes)
	{
		m_pdrgpszPartTypes = CDXLOperatorFactory::PdrgpszFromXMLCh
														(
														m_parse_handler_mgr->Pmm(),
														xmlszPartTypes,
														EdxltokenPartTypes,
														EdxltokenRelation
														);
	}

	const XMLCh *xmlszPartitions = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenNumLeafPartitions));

	if (NULL != xmlszPartitions)
	{
		m_ulPartitions = CDXLOperatorFactory::UlValueFromXmlstr
														(
														m_parse_handler_mgr->Pmm(),
														xmlszPartitions,
														EdxltokenNumLeafPartitions,
														EdxltokenRelation
														);
	}

	// parse whether a hash distributed relation needs to be considered as random distributed
	const XMLCh *xmlszConvertHashToRandom = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenConvertHashToRandom));
	if (NULL != xmlszConvertHashToRandom)
	{
		m_fConvertHashToRandom = CDXLOperatorFactory::FValueFromXmlstr
										(
										m_parse_handler_mgr->Pmm(), 
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
			CDXLNode *pdxlnPartConstraint = pphPartCnstr->Pdxln();
			pdxlnPartConstraint->AddRef();
			m_ppartcnstr = GPOS_NEW(m_memory_pool) CMDPartConstraintGPDB(m_memory_pool, m_pdrgpulDefaultParts, m_fPartConstraintUnbounded, pdxlnPartConstraint);
		}
		else
		{
			// no partition constraint expression
			m_ppartcnstr = GPOS_NEW(m_memory_pool) CMDPartConstraintGPDB(m_memory_pool, m_pdrgpulDefaultParts, m_fPartConstraintUnbounded, NULL);
		}
		return;
	}

	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelation), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// construct metadata object from the created child elements
	CParseHandlerMetadataColumns *pphMdCol = dynamic_cast<CParseHandlerMetadataColumns *>((*this)[0]);
	CParseHandlerMetadataIdList *pphMdidlTriggers = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[2]);
	CParseHandlerMetadataIdList *pphMdidlCheckConstraints = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[3]);

	GPOS_ASSERT(NULL != pphMdCol->Pdrgpmdcol());
	GPOS_ASSERT(NULL != pphMdlIndexInfo->PdrgpmdIndexInfo());
	GPOS_ASSERT(NULL != pphMdidlCheckConstraints->GetMdIdArray());

	// refcount child objects
	DrgPmdcol *pdrgpmdcol = pphMdCol->Pdrgpmdcol();
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
									m_fTemporary,
									m_erelstorage,
									m_ereldistrpolicy,
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
									m_ppartcnstr,
									m_fHasOids
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
	const XMLCh *xmlszTableName = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenName, edxltokenElement);
	CWStringDynamic *pstrTableName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlszTableName);
	m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrTableName);
	GPOS_DELETE(pstrTableName);

	// parse metadata id info
	m_mdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenMdid, edxltokenElement);

	// parse distribution policy
	const XMLCh *xmlszDistrPolicy = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenRelDistrPolicy, edxltokenElement);
	m_ereldistrpolicy = CDXLOperatorFactory::EreldistrpolicyFromXmlstr(xmlszDistrPolicy);

	if (m_ereldistrpolicy == IMDRelation::EreldistrHash)
	{
		// parse distribution columns
		const XMLCh *xmlszDistrColumns = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDistrColumns, edxltokenElement);
		m_pdrgpulDistrColumns = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDistrColumns, EdxltokenDistrColumns, edxltokenElement);
	}

	// parse keys
	const XMLCh *xmlszKeys = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenKeys));
	if (NULL != xmlszKeys)
	{
		m_pdrgpdrgpulKeys = CDXLOperatorFactory::PdrgpdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszKeys, EdxltokenKeys, edxltokenElement);
	}
	else
	{
		// construct an empty keyset
		m_pdrgpdrgpulKeys = GPOS_NEW(m_memory_pool) ULongPtrArray2D(m_memory_pool);
	}

	m_ulPartitions = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenNumLeafPartitions, edxltokenElement,
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
