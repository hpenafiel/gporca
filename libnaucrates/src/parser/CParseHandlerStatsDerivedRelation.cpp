//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerStatsDerivedRelation.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing derived relation
//		statistics.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerStatsDerivedRelation.h"
#include "naucrates/dxl/parser/CParseHandlerStatsDerivedColumn.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
using namespace gpnaucrates;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsDerivedRelation::CParseHandlerStatsDerivedRelation
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerStatsDerivedRelation::CParseHandlerStatsDerivedRelation
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(pmp, parse_handler_mgr, pphRoot),
	m_dRows(CStatistics::DDefaultColumnWidth),
	m_fEmpty(false),
	m_pdxlstatsderrel(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsDerivedRelation::CParseHandlerStatsDerivedRelation
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerStatsDerivedRelation::~CParseHandlerStatsDerivedRelation()
{
	m_pdxlstatsderrel->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsDerivedRelation::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerStatsDerivedRelation::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsDerivedColumn), element_local_name))
	{
		// start new derived column element
		CParseHandlerBase *pph = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenStatsDerivedColumn), m_pphm, this);
		m_pphm->ActivateParseHandler(pph);

		// store parse handler
		this->Append(pph);

		pph->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		GPOS_ASSERT(0 == this->Length());

		// parse rows
		const XMLCh *xmlszRows = CDXLOperatorFactory::XmlstrFromAttrs
														(
														attrs,
														EdxltokenRows,
														EdxltokenStatsDerivedRelation
														);

		m_dRows = CDouble(CDXLOperatorFactory::DValueFromXmlstr
												(
												m_pphm->Pmm(),
												xmlszRows,
												EdxltokenRows,
												EdxltokenStatsDerivedRelation
												));

		m_fEmpty = false;
		const XMLCh *xmlszEmpty = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenEmptyRelation));
		if (NULL != xmlszEmpty)
		{
			m_fEmpty = CDXLOperatorFactory::FValueFromXmlstr
											(
											m_pphm->Pmm(),
											xmlszEmpty,
											EdxltokenEmptyRelation,
											EdxltokenStatsDerivedRelation
											);
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerStatsDerivedRelation::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerStatsDerivedRelation::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenStatsDerivedRelation), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// must have at least one column stats
	GPOS_ASSERT(0 < this->Length());

	// array of derived column statistics
	DrgPdxlstatsdercol *pdrgpdxlstatsdercol = GPOS_NEW(m_pmp) DrgPdxlstatsdercol(m_pmp);
	const ULONG ulDerCol = this->Length();
	for (ULONG ul = 0; ul < ulDerCol; ul++)
	{
		CParseHandlerStatsDerivedColumn *pph = dynamic_cast<CParseHandlerStatsDerivedColumn*>( (*this)[ul]);

		CDXLStatsDerivedColumn *pdxlstatdercol = pph->Pstatsdercol();
		pdxlstatdercol->AddRef();
		pdrgpdxlstatsdercol->Append(pdxlstatdercol);
	}

	m_pdxlstatsderrel = GPOS_NEW(m_pmp) CDXLStatsDerivedRelation(m_dRows, m_fEmpty, pdrgpdxlstatsdercol);

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
