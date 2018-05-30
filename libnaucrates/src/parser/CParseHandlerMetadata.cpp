//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMetadata.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing a DXL document.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMetadata.h"

#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataObject.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include <xercesc/util/XMLStringTokenizer.hpp>

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::CParseHandlerMetadata
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMetadata::CParseHandlerMetadata
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(pmp, parse_handler_mgr, pphRoot),
	m_pdrgpmdobj(NULL),
	m_pdrgpmdid(NULL),
	m_pdrgpsysid(NULL)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::~CParseHandlerMetadata
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerMetadata::~CParseHandlerMetadata()
{
	CRefCount::SafeRelease(m_pdrgpmdobj);
	CRefCount::SafeRelease(m_pdrgpmdid);
	CRefCount::SafeRelease(m_pdrgpsysid);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::Edxlphtype
//
//	@doc:
//		Return the type of the parse handler. Currently we overload this method to 
//		return a specific type for the plann, query and metadata parse handlers.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerMetadata::Edxlphtype() const
{
	return EdxlphMetadata;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::Pdrgpmdobj
//
//	@doc:
//		Returns the list of metadata objects constructed by the parser
//
//---------------------------------------------------------------------------
DrgPimdobj *
CParseHandlerMetadata::Pdrgpmdobj()
{
	return m_pdrgpmdobj;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::Pdrgpmdid
//
//	@doc:
//		Returns the list of metadata ids constructed by the parser
//
//---------------------------------------------------------------------------
DrgPmdid *
CParseHandlerMetadata::Pdrgpmdid()
{
	return m_pdrgpmdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::Pdrgpsysid
//
//	@doc:
//		Returns the list of metadata source system ids constructed by the parser
//
//---------------------------------------------------------------------------
DrgPsysid *
CParseHandlerMetadata::Pdrgpsysid()
{
	return m_pdrgpsysid;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMetadata::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{		
	if(0 == XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenMetadata)))
	{
		// start of the metadata section in the DXL document
		GPOS_ASSERT(NULL == m_pdrgpmdobj);
		
		m_pdrgpmdobj = GPOS_NEW(m_memory_pool) DrgPimdobj(m_memory_pool);
		m_pdrgpmdid = GPOS_NEW(m_memory_pool) DrgPmdid(m_memory_pool);
		
		m_pdrgpsysid = PdrgpsysidParse
						(
						attrs, 
						EdxltokenSysids,
						EdxltokenMetadata
						);
	}
	else if (0 == XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenMdid)))
	{
		// start of the metadata section in the DXL document
		GPOS_ASSERT(NULL != m_pdrgpmdid);
		IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs(m_pphm->Pmm(), attrs, EdxltokenValue, EdxltokenMdid);
		m_pdrgpmdid->Append(pmdid);
	}
	else
	{
		// must be a metadata object
		GPOS_ASSERT(NULL != m_pdrgpmdobj);
		
		// install a parse handler for the given element
		CParseHandlerBase *pph = CParseHandlerFactory::Pph(m_memory_pool, element_local_name, m_pphm, this);

		m_pphm->ActivateParseHandler(pph);
		
		// store parse handler
		this->Append(pph);
		
		pph->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMetadata::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenMetadata)) &&
		0 != XMLString::compareString(element_local_name, CDXLTokens::XmlstrToken(EdxltokenMdid)))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	GPOS_ASSERT(NULL != m_pdrgpmdobj);
	
	const ULONG ulLen = this->Length();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CParseHandlerMetadataObject *pphMdObj = dynamic_cast<CParseHandlerMetadataObject *>((*this)[ul]);

		GPOS_ASSERT(NULL != pphMdObj->Pimdobj());

		IMDCacheObject *pimdobj = pphMdObj->Pimdobj();
		pimdobj->AddRef();
		m_pdrgpmdobj->Append(pimdobj);
	}

	m_pphm->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadata::PdrgpsysidParse
//
//	@doc:
//		Parse a list of source system ids
//
//---------------------------------------------------------------------------
DrgPsysid *
CParseHandlerMetadata::PdrgpsysidParse
	(
	const Attributes &attrs,
	Edxltoken edxltokenAttr,
	Edxltoken edxltokenElement
	)
{	
	const XMLCh *xmlszAttrName = CDXLTokens::XmlstrToken(edxltokenAttr);

	// extract systemids
	const XMLCh *xmlsz = attrs.getValue(xmlszAttrName);
	
	if (NULL == xmlsz)
	{
		return NULL;
	}

	DrgPsysid *pdrgpsysid = GPOS_NEW(m_memory_pool) DrgPsysid(m_memory_pool);

	// extract separate system ids 
	XMLStringTokenizer xmlsztok(xmlsz, CDXLTokens::XmlstrToken(EdxltokenComma));
	
	XMLCh *xmlszSysId = NULL;
	while (NULL != (xmlszSysId = xmlsztok.nextToken()))
	{
		// get sysid components
		XMLStringTokenizer xmlsztokSysid(xmlszSysId, CDXLTokens::XmlstrToken(EdxltokenDot));
		GPOS_ASSERT(2 == xmlsztokSysid.countTokens());
		
		XMLCh *xmlszType = xmlsztokSysid.nextToken();
		ULONG ulType = CDXLOperatorFactory::UlValueFromXmlstr(m_pphm->Pmm(), xmlszType, edxltokenAttr, edxltokenElement);
		
		XMLCh *xmlszName = xmlsztokSysid.nextToken();
		CWStringDynamic *pstrName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlszName);
		
		pdrgpsysid->Append(GPOS_NEW(m_memory_pool) CSystemId((IMDId::EMDIdType) ulType, pstrName->GetBuffer(), pstrName->Length()));	
		
		GPOS_DELETE(pstrName);
	}
	
	return pdrgpsysid;
}

// EOF
