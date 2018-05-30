//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalTVF.cpp
//
//	@doc:
//
//		Implementation of the SAX parse handler class for parsing table-valued
//		functions
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalTVF.h"
#include "naucrates/dxl/parser/CParseHandlerColDescr.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalOp.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalTVF::CParseHandlerLogicalTVF
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalTVF::CParseHandlerLogicalTVF
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, pphRoot),
	m_pmdidFunc(NULL),
	m_pmdidRetType(NULL),
	m_mdname(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalTVF::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalTVF::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes &attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalTVF), element_local_name))
	{
		// parse function id
		m_pmdidFunc = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenFuncId, EdxltokenLogicalTVF);

		// parse function name
		const XMLCh *xmlszFuncName = CDXLOperatorFactory::XmlstrFromAttrs
																(
																attrs,
																EdxltokenName,
																EdxltokenLogicalTVF
																);

		CWStringDynamic *pstrFuncName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), xmlszFuncName);
		m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrFuncName);
		GPOS_DELETE(pstrFuncName);

		// parse function return type
		m_pmdidRetType = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeId, EdxltokenLogicalTVF);

	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumns), element_local_name))
	{
		// parse handler for columns
		CParseHandlerBase *pphColDescr = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenColumns), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphColDescr);

		// store parse handlers
		this->Append(pphColDescr);

		pphColDescr->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		// parse scalar child
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handlers
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalTVF::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalTVF::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalTVF), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerColDescr *pphColDescr = dynamic_cast<CParseHandlerColDescr *>((*this)[0]);

	GPOS_ASSERT(NULL != pphColDescr);

	// get column descriptors
	ColumnDescrDXLArray *pdrgpdxlcd = pphColDescr->GetColumnDescrDXLArray();
	GPOS_ASSERT(NULL != pdrgpdxlcd);

	pdrgpdxlcd->AddRef();
	CDXLLogicalTVF *pdxlopTVF = GPOS_NEW(m_memory_pool) CDXLLogicalTVF(m_memory_pool, m_pmdidFunc, m_pmdidRetType, m_mdname, pdrgpdxlcd);

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopTVF);

	const ULONG ulLen = this->Length();
	// loop over arglist children and add them to this parsehandler
	for (ULONG ul = 1; ul < ulLen; ul++)
	{
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(pphChild);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
