//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerWindowSpec.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing the
//		window specification node
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerWindowSpec.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerWindowFrame.h"
#include "naucrates/dxl/parser/CParseHandlerSortColList.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowSpec::CParseHandlerWindowSpec
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerWindowSpec::CParseHandlerWindowSpec
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, pphRoot),
	m_pdrgpulPartCols(NULL),
	m_pdxlws(NULL),
	m_pmdname(NULL),
	m_fHasWindowFrame(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowSpec::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowSpec::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowSpec), element_local_name))
	{
		GPOS_ASSERT(0 == this->Length());
		GPOS_ASSERT(NULL == m_pdxlws);
		GPOS_ASSERT(NULL == m_pmdname);

		// parse alias from attributes
		const XMLCh *xmlszAlias = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenAlias));
		if (NULL != xmlszAlias)
		{
			CWStringDynamic *pstrAlias = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlszAlias);
			m_pmdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrAlias);
			GPOS_DELETE(pstrAlias);
		}

		const XMLCh *xmlszPartCols= CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenPartKeys, EdxltokenPhysicalWindow);
		m_pdrgpulPartCols = CDXLOperatorFactory::PdrgpulFromXMLCh(m_pphm->Pmm(), xmlszPartCols, EdxltokenPartKeys, EdxltokenPhysicalWindow);
		GPOS_ASSERT(NULL != m_pdrgpulPartCols);

	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSortColList), element_local_name))
	{
		// parse handler for the sorting column list
		CParseHandlerBase *pphSortColList =
					CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSortColList), m_pphm, this);
		m_pphm->ActivateParseHandler(pphSortColList);

		// store parse handler
		this->Append(pphSortColList);
		pphSortColList->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowFrame), element_local_name))
	{
		m_fHasWindowFrame = true;

		// parse handler for the leading and trailing scalar values
		CParseHandlerBase *pphWf =
				CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenWindowFrame), m_pphm, this);
		m_pphm->ActivateParseHandler(pphWf);

		// store parse handler
		this->Append(pphWf);
		pphWf->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
			CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerWindowSpec::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerWindowSpec::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenWindowSpec), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	// sorting columns
	CDXLNode *pdxlnSortColList = NULL;

	// window frame associated with the window key
	CDXLWindowFrame *pdxlwf =  NULL;

	if (1 == this->Length())
	{
		if (m_fHasWindowFrame)
		{
			// In GPDB 5 and before, window specification cannot have a window frame specification
			// without sort columns. This changed in GPDB 6/Postgres 8.4+ where a query
			// select b,c, count(c) over (partition by b) from (select * from foo) s;
			// adds a window frame that is unbounded.
			CParseHandlerWindowFrame *pphWf = dynamic_cast<CParseHandlerWindowFrame *>((*this)[0]);
			pdxlwf = pphWf->Pdxlwf();
		}
		else
		{
			CParseHandlerSortColList *pphSortColList = dynamic_cast<CParseHandlerSortColList*>((*this)[0]);
			pdxlnSortColList = pphSortColList->Pdxln();
			pdxlnSortColList->AddRef();
		}
	}
	else if (2 == this->Length())
	{
		CParseHandlerSortColList *pphSortColList = dynamic_cast<CParseHandlerSortColList*>((*this)[0]);
		pdxlnSortColList = pphSortColList->Pdxln();
		pdxlnSortColList->AddRef();

		CParseHandlerWindowFrame *pphWf = dynamic_cast<CParseHandlerWindowFrame *>((*this)[1]);
		pdxlwf = pphWf->Pdxlwf();
	}
	m_pdxlws = GPOS_NEW(m_memory_pool) CDXLWindowSpec(m_memory_pool, m_pdrgpulPartCols, m_pmdname, pdxlnSortColList, pdxlwf);

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
