//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerMDRelationExternal.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing external
//		relation metadata.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDRelationExternal.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataColumns.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerMDIndexInfoList.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#define GPDXL_DEFAULT_REJLIMIT -1

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelationExternal::CParseHandlerMDRelationExternal
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDRelationExternal::CParseHandlerMDRelationExternal
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMDRelation(memory_pool, parse_handler_mgr, parse_handler_root),
	m_iRejectLimit(GPDXL_DEFAULT_REJLIMIT),
	m_fRejLimitInRows(false),
	m_pmdidFmtErrRel(NULL)
{
	m_rel_storage_type = IMDRelation::ErelstorageExternal;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelationExternal::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelationExternal::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelationExternal), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// parse main relation attributes: name, id, distribution policy and keys
	ParseRelationAttributes(attrs, EdxltokenRelation);

	// parse reject limit
	const XMLCh *xmlszRejLimit = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenExtRelRejLimit));
	if (NULL != xmlszRejLimit)
	{
		m_iRejectLimit = CDXLOperatorFactory::ConvertAttrValueToInt(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszRejLimit, EdxltokenExtRelRejLimit, EdxltokenRelationExternal);
		m_fRejLimitInRows = CDXLOperatorFactory::ExtractConvertAttrValueToBool(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenExtRelRejLimitInRows, EdxltokenRelationExternal);
	}

	// format error table id
	const XMLCh *xmlszErrRel = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenExtRelFmtErrRel));
	if (NULL != xmlszErrRel)
	{
		m_pmdidFmtErrRel = CDXLOperatorFactory::MakeMdIdFromStr(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszErrRel, EdxltokenExtRelFmtErrRel, EdxltokenRelationExternal);
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
										EdxltokenRelationExternal
										);
	}

	// parse children
	ParseChildNodes();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDRelationExternal::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDRelationExternal::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenRelationExternal), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// construct metadata object from the created child elements
	CParseHandlerMetadataColumns *pphMdCol = dynamic_cast<CParseHandlerMetadataColumns *>((*this)[0]);
	CParseHandlerMDIndexInfoList *pphMdlIndexInfo = dynamic_cast<CParseHandlerMDIndexInfoList*>((*this)[1]);
	CParseHandlerMetadataIdList *pphMdidlTriggers = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[2]);
	CParseHandlerMetadataIdList *pphMdidlCheckConstraints = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[3]);

	GPOS_ASSERT(NULL != pphMdCol->Pdrgpmdcol());
	GPOS_ASSERT(NULL != pphMdlIndexInfo->PdrgpmdIndexInfo());
	GPOS_ASSERT(NULL != pphMdidlCheckConstraints->GetMdIdArray());

	// refcount child objects
	DrgPmdcol *pdrgpmdcol = pphMdCol->Pdrgpmdcol();
	DrgPmdIndexInfo *pdrgpmdIndexInfo = pphMdlIndexInfo->PdrgpmdIndexInfo();
	DrgPmdid *pdrgpmdidTriggers = pphMdidlTriggers->GetMdIdArray();
	DrgPmdid *pdrgpmdidCheckConstraint = pphMdidlCheckConstraints->GetMdIdArray();

	pdrgpmdcol->AddRef();
	pdrgpmdIndexInfo->AddRef();
 	pdrgpmdidTriggers->AddRef();
 	pdrgpmdidCheckConstraint->AddRef();

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDRelationExternalGPDB
								(
									m_memory_pool,
									m_mdid,
									m_mdname,
									m_rel_distr_policy,
									pdrgpmdcol,
									m_pdrgpulDistrColumns,
									m_fConvertHashToRandom,
									m_pdrgpdrgpulKeys,
									pdrgpmdIndexInfo,
									pdrgpmdidTriggers,
									pdrgpmdidCheckConstraint,
									m_iRejectLimit,
									m_fRejLimitInRows,
									m_pmdidFmtErrRel
								);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
