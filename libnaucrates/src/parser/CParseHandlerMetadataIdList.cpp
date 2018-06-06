//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMetadataIdList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing lists of metadata ids,
//		for example in the specification of the indices or partition tables for a relation.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::CParseHandlerMetadataIdList
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMetadataIdList::CParseHandlerMetadataIdList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid_array(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::~CParseHandlerMetadataIdList
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerMetadataIdList::~CParseHandlerMetadataIdList()
{
	CRefCount::SafeRelease(m_mdid_array);
}



//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMetadataIdList::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (FSupportedListType(element_local_name))
	{
		// start of an index or partition metadata id list
		GPOS_ASSERT(NULL == m_mdid_array);
		
		m_mdid_array = GPOS_NEW(m_memory_pool) DrgPmdid(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndex), element_local_name))
	{
		// index metadata id: array must be initialized already
		GPOS_ASSERT(NULL != m_mdid_array);
		
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenIndex);
		m_mdid_array->Append(pmdid);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenTrigger), element_local_name))
	{
		// trigger metadata id: array must be initialized already
		GPOS_ASSERT(NULL != m_mdid_array);

		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenTrigger);
		m_mdid_array->Append(pmdid);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartition), element_local_name))
	{
		// partition metadata id: array must be initialized already
		GPOS_ASSERT(NULL != m_mdid_array);
		
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenPartition);
		m_mdid_array->Append(pmdid);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCheckConstraint), element_local_name))
	{
		// check constraint metadata id: array must be initialized already
		GPOS_ASSERT(NULL != m_mdid_array);
		
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenCheckConstraint);
		m_mdid_array->Append(pmdid);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpClass), element_local_name))
	{
		// opclass metadata id: array must be initialized already
		GPOS_ASSERT(NULL != m_mdid_array);
		
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenOpClass);
		m_mdid_array->Append(pmdid);
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMetadataIdList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenTriggers), element_local_name) ||
		0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartitions), element_local_name)||
		0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCheckConstraints), element_local_name) ||
		0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpClasses), element_local_name))
	{
		// end the index or partition metadata id list
		GPOS_ASSERT(NULL != m_mdid_array);

		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (!FSupportedElem(element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::FSupportedElem
//
//	@doc:
//		Is this a supported MD list elem
//
//---------------------------------------------------------------------------
BOOL
CParseHandlerMetadataIdList::FSupportedElem
	(
	const XMLCh* const xmlsz
	)
{
	return (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenIndex), xmlsz) ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenTrigger), xmlsz)  ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartition), xmlsz)  ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCheckConstraint), xmlsz)  ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpClass), xmlsz));
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::FSupportedListType
//
//	@doc:
//		Is this a supported MD list type
//
//---------------------------------------------------------------------------
BOOL
CParseHandlerMetadataIdList::FSupportedListType
	(
	const XMLCh* const xmlsz
	)
{
	return (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenTriggers), xmlsz)  ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartitions), xmlsz)  ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCheckConstraints), xmlsz)  ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpClasses), xmlsz));
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataIdList::GetMdIdArray
//
//	@doc:
//		Return the constructed list of metadata ids
//
//---------------------------------------------------------------------------
DrgPmdid *
CParseHandlerMetadataIdList::GetMdIdArray()
{
	return m_mdid_array;
}
// EOF
