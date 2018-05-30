//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalSetOp.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		set operators.
//
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalSetOp.h"
#include "naucrates/dxl/parser/CParseHandlerColDescr.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSetOp::CParseHandlerLogicalSetOp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalSetOp::CParseHandlerLogicalSetOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, pphRoot),
	m_edxlsetop(EdxlsetopSentinel),
	m_pdrgpdrgpulInputColIds(NULL),
	m_fCastAcrossInputs(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSetOp::~CParseHandlerLogicalSetOp
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalSetOp::~CParseHandlerLogicalSetOp()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSetOp::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalSetOp::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{

	if (0 == this->Length())
	{
		m_edxlsetop = Edxlsetop(element_local_name);

		if (EdxlsetopSentinel == m_edxlsetop)
		{
			GPOS_RAISE
				(
				gpdxl::ExmaDXL,
				gpdxl::ExmiDXLUnexpectedTag,
				CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name)->GetBuffer()
				);
		}

		// parse array of input colid arrays
		const XMLCh *xmlszInputColIds = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenInputCols));
		m_pdrgpdrgpulInputColIds = CDXLOperatorFactory::PdrgpdrgpulFromXMLCh(m_pphm->Pmm(), xmlszInputColIds, EdxltokenInputCols, EdxltokenLogicalSetOperation);

		// install column descriptor parsers
		CParseHandlerBase *pphColDescr = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenColumns), m_pphm, this);
		m_pphm->ActivateParseHandler(pphColDescr);

		m_fCastAcrossInputs = CDXLOperatorFactory::FValueFromAttrs(m_pphm->Pmm(), attrs, EdxltokenCastAcrossInputs, EdxltokenLogicalSetOperation);

		// store child parse handler in array
		this->Append(pphColDescr);
	}
	else
	{
		// already have seen a set operation
		GPOS_ASSERT(EdxlsetopSentinel != m_edxlsetop);

		// create child node parsers
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_pphm, this);
		m_pphm->ActivateParseHandler(pphChild);

		this->Append(pphChild);
		
		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSetOp::Edxlsetop
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
EdxlSetOpType
CParseHandlerLogicalSetOp::Edxlsetop
	(
	const XMLCh* const element_local_name
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalUnion), element_local_name))
	{
		return EdxlsetopUnion;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalUnionAll), element_local_name))
	{
		return EdxlsetopUnionAll;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalIntersect), element_local_name))
	{
		return EdxlsetopIntersect;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalIntersectAll), element_local_name))
	{
		return EdxlsetopIntersectAll;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalDifference), element_local_name))
	{
		return EdxlsetopDifference;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalDifferenceAll), element_local_name))
	{
		return EdxlsetopDifferenceAll;
	}

	return EdxlsetopSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalSetOp::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalSetOp::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EdxlSetOpType edxlsetop = Edxlsetop(element_local_name);

	if(EdxlsetopSentinel == edxlsetop && m_edxlsetop != edxlsetop)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulLen = this->Length();
	GPOS_ASSERT(3 <= ulLen);

	// get the columns descriptors
	CParseHandlerColDescr *pphColDescr = dynamic_cast<CParseHandlerColDescr *>((*this)[0]);
	GPOS_ASSERT(NULL != pphColDescr->GetColumnDescrDXLArray());
	ColumnDescrDXLArray *pdrgpdxlcd = pphColDescr->GetColumnDescrDXLArray();

	pdrgpdxlcd->AddRef();
	CDXLLogicalSetOp *pdxlop = GPOS_NEW(m_memory_pool) CDXLLogicalSetOp(m_memory_pool, edxlsetop, pdrgpdxlcd, m_pdrgpdrgpulInputColIds, m_fCastAcrossInputs);
	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlop);

	for (ULONG ul = 1; ul < ulLen; ul++)
	{
		// add constructed logical children from child parse handlers
		CParseHandlerLogicalOp *pphChild = dynamic_cast<CParseHandlerLogicalOp*>((*this)[ul]);
		AddChildFromParseHandler(pphChild);
	}		
	
#ifdef GPOS_DEBUG
	m_pdxln->Pdxlop()->AssertValid(m_pdxln, false /* fValidateChildren */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_pphm->DeactivateHandler();
}
// EOF
