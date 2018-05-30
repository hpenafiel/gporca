//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerMDIndex.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing an MD index
//---------------------------------------------------------------------------

#include "naucrates/md/CMDIndexGPDB.h"

#include "naucrates/dxl/parser/CParseHandlerMDIndex.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDIndex::CParseHandlerMDIndex
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDIndex::CParseHandlerMDIndex
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, pphRoot),
	m_mdid(NULL),
	m_mdname(NULL),
	m_fClustered(false),
	m_emdindt(IMDIndex::EmdindSentinel),
	m_pmdidItemType(NULL),
	m_pdrgpulKeyCols(NULL),
	m_pdrgpulIncludedCols(NULL),
	m_ppartcnstr(NULL),
	m_pdrgpulDefaultParts(NULL),
	m_fPartConstraintUnbounded(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDIndex::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDIndex::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartConstraint), element_local_name))
	{
		GPOS_ASSERT(NULL == m_ppartcnstr);
		
		const XMLCh *xmlszDefParts = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenDefaultPartition));
		if (NULL != xmlszDefParts)
		{
			m_pdrgpulDefaultParts = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDefParts, EdxltokenDefaultPartition, EdxltokenIndex);
		}
		else
		{
			// construct an empty keyset
			m_pdrgpulDefaultParts = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
		}

		m_fPartConstraintUnbounded = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenPartConstraintUnbounded, EdxltokenIndex);

		// parse handler for part constraints
		CParseHandlerBase *pphPartConstraint= CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphPartConstraint);
		this->Append(pphPartConstraint);
		return;
	}
	
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndex), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// new index object 
	GPOS_ASSERT(NULL == m_mdid);

	// parse mdid
	m_mdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenMdid, EdxltokenIndex);
	
	// parse index name
	const XMLCh *parsed_column_name = CDXLOperatorFactory::XmlstrFromAttrs
															(
															attrs,
															EdxltokenName,
															EdxltokenIndex
															);
	CWStringDynamic *column_name = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), parsed_column_name);
	
	// create a copy of the string in the CMDName constructor
	m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, column_name);
	GPOS_DELETE(column_name);

	// parse index clustering, key columns and included columns information
	m_fClustered = CDXLOperatorFactory::FValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenIndexClustered, EdxltokenIndex);
	
	m_emdindt = CDXLOperatorFactory::EmdindtFromAttr(attrs);
	const XMLCh *xmlszItemType = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenIndexItemType));
	if (NULL != xmlszItemType)
	{
		m_pmdidItemType = CDXLOperatorFactory::PmdidFromXMLCh
							(
							m_parse_handler_mgr->Pmm(),
							xmlszItemType,
							EdxltokenIndexItemType,
							EdxltokenIndex
							);
	}

	const XMLCh *xmlszIndexKeys = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenIndexKeyCols, EdxltokenIndex);
	m_pdrgpulKeyCols = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszIndexKeys, EdxltokenIndexKeyCols, EdxltokenIndex);

	const XMLCh *xmlszIndexIncludedCols = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenIndexIncludedCols, EdxltokenIndex);
	m_pdrgpulIncludedCols = CDXLOperatorFactory::PdrgpulFromXMLCh
													(
													m_parse_handler_mgr->Pmm(),
													xmlszIndexIncludedCols,
													EdxltokenIndexIncludedCols,
													EdxltokenIndex
													);
	
	// parse handler for operator class list
	CParseHandlerBase *pphOpClassList = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenMetadataIdList), m_parse_handler_mgr, this);
	this->Append(pphOpClassList);
	m_parse_handler_mgr->ActivateParseHandler(pphOpClassList);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDIndex::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDIndex::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartConstraint), element_local_name))
	{
		GPOS_ASSERT(2 == Length());
		
		CParseHandlerScalarOp *pphPartCnstr = dynamic_cast<CParseHandlerScalarOp *>((*this)[1]);
		CDXLNode *pdxlnPartConstraint = pphPartCnstr->Pdxln();
		pdxlnPartConstraint->AddRef();
		m_ppartcnstr = GPOS_NEW(m_memory_pool) CMDPartConstraintGPDB(m_memory_pool, m_pdrgpulDefaultParts, m_fPartConstraintUnbounded, pdxlnPartConstraint);
		return;
	}
	
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndex), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	CParseHandlerMetadataIdList *pphMdidOpClasses = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[0]);
	DrgPmdid *pdrgpmdidOpClasses = pphMdidOpClasses->Pdrgpmdid();
	pdrgpmdidOpClasses->AddRef();

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDIndexGPDB
							(
							m_memory_pool, 
							m_mdid, 
							m_mdname,
							m_fClustered, 
							m_emdindt,
							m_pmdidItemType,
							m_pdrgpulKeyCols, 
							m_pdrgpulIncludedCols, 
							pdrgpmdidOpClasses,
							m_ppartcnstr
							);
	
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
