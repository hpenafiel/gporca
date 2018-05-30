//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerPartitionSelector.cpp
//
//	@doc:
//
//		Implementation of the SAX parse handler class for parsing partition
//		selectors
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerPartitionSelector.h"
#include "naucrates/dxl/parser/CParseHandlerProperties.h"
#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerUtils.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPartitionSelector::CParseHandlerPartitionSelector
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerPartitionSelector::CParseHandlerPartitionSelector
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerPhysicalOp(pmp, parse_handler_mgr, pphRoot),
	m_pmdidRel(NULL),
	m_ulLevels(0),
	m_ulScanId(0)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPartitionSelector::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerPartitionSelector::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes &attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalPartitionSelector), element_local_name))
	{
		// PartitionSelector node may have another PartitionSelector node as a child
		if (NULL != m_pmdidRel)
		{
			// instantiate the parse handler
			CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_pmp, element_local_name, m_pphm, this);
			
			GPOS_ASSERT(NULL != pphChild);
			
			// activate the parse handler
			m_pphm->ActivateParseHandler(pphChild);
			this->Append(pphChild);
			
			// pass the startElement message for the specialized parse handler to process
			pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
		}
		else
		{
			// parse table id
			m_pmdidRel = CDXLOperatorFactory::PmdidFromAttrs(m_pphm->Pmm(), attrs, EdxltokenRelationMdid, EdxltokenPhysicalPartitionSelector);
			
			// parse number of levels
			m_ulLevels = CDXLOperatorFactory::UlValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenPhysicalPartitionSelectorLevels, EdxltokenPhysicalPartitionSelector);
			
			// parse scan id
			m_ulScanId = CDXLOperatorFactory::UlValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenPhysicalPartitionSelectorScanId, EdxltokenPhysicalPartitionSelector);
			
			// parse handlers for all the scalar children
			CParseHandlerBase *pphOpListFilters = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalarOpList), m_pphm, this);
			m_pphm->ActivateParseHandler(pphOpListFilters);
			
			CParseHandlerBase *pphOpListEqFilters = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalarOpList), m_pphm, this);
			m_pphm->ActivateParseHandler(pphOpListEqFilters);
			
			// parse handler for the proj list
			CParseHandlerBase *pphPrL = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalarProjList), m_pphm, this);
			m_pphm->ActivateParseHandler(pphPrL);
			
			// parse handler for the properties of the operator
			CParseHandlerBase *pphProp = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenProperties), m_pphm, this);
			m_pphm->ActivateParseHandler(pphProp);
			
			// store parse handlers
			this->Append(pphProp);
			this->Append(pphPrL);
			this->Append(pphOpListEqFilters);
			this->Append(pphOpListFilters);
		}
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarResidualFilter), element_local_name))
	{
		CParseHandlerBase *pphResidual = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);
		m_pphm->ActivateParseHandler(pphResidual);
		this->Append(pphResidual);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPropagationExpr), element_local_name))
	{
		CParseHandlerBase *pphPropagation = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);
		m_pphm->ActivateParseHandler(pphPropagation);
		this->Append(pphPropagation);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPrintableFilter), element_local_name))
	{
		CParseHandlerBase *pphPrintFilter = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenScalar), m_pphm, this);
		m_pphm->ActivateParseHandler(pphPrintFilter);
		this->Append(pphPrintFilter);
	}
	else
	{
		// parse physical child
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_pmp, CDXLTokens::XmlstrToken(EdxltokenPhysical), m_pphm, this);
		m_pphm->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerPartitionSelector::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerPartitionSelector::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarResidualFilter), element_local_name) ||
		0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPropagationExpr), element_local_name) ||
		0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarPrintableFilter), element_local_name))
	{
		return;
	}

	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPhysicalPartitionSelector), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CDXLPhysicalPartitionSelector *pdxlop = GPOS_NEW(m_pmp) CDXLPhysicalPartitionSelector(m_pmp, m_pmdidRel, m_ulLevels, m_ulScanId);
	m_pdxln = GPOS_NEW(m_pmp) CDXLNode(m_pmp, pdxlop);

	CParseHandlerProperties *pphProp = dynamic_cast<CParseHandlerProperties *>((*this)[0]);

	// set statistics and physical properties
	CParseHandlerUtils::SetProperties(m_pdxln, pphProp);

	// scalar children
	for (ULONG ul = 1; ul < 7; ul++)
	{
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(pphChild);
	}

	// optional physical child
	if (8 == this->Length())
	{
		CParseHandlerPhysicalOp *pphChild = dynamic_cast<CParseHandlerPhysicalOp *>((*this)[7]);
		AddChildFromParseHandler(pphChild);
	}

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF

