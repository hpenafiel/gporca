//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerScalarArrayRef.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing arrayref
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarArrayRef.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLScalarArrayRef.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarArrayRef::CParseHandlerScalarArrayRef
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarArrayRef::CParseHandlerScalarArrayRef
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, pphRoot),
	m_ulIndexLists(0),
	m_fParsingRefExpr(false),
	m_fParsingAssignExpr(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarArrayRef::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarArrayRef::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRef), element_local_name))
	{
		// initialize the arrayref node
		GPOS_ASSERT(NULL == m_pdxln);

		// parse types
		IMDId *pmdidElem = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenArrayElementType, EdxltokenScalarArrayRef);
		IMDId *pmdidArray = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenArrayType, EdxltokenScalarArrayRef);
		IMDId *pmdidReturn = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeId, EdxltokenScalarArrayRef);
		INT type_modifier = CDXLOperatorFactory::IValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTypeMod, EdxltokenScalarArrayRef, true, IDefaultTypeModifier);

		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode (m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarArrayRef(m_memory_pool, pmdidElem, type_modifier, pmdidArray, pmdidReturn));
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRefIndexList), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pdxln);
		GPOS_ASSERT(2 > m_ulIndexLists);

		// parse index list
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarArrayRefIndexList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);
		m_ulIndexLists++;

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRefExpr), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pdxln);
		GPOS_ASSERT(2 == m_ulIndexLists);
		GPOS_ASSERT(!m_fParsingRefExpr);
		GPOS_ASSERT(!m_fParsingAssignExpr);

		m_fParsingRefExpr = true;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRefAssignExpr), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pdxln);
		GPOS_ASSERT(2 == m_ulIndexLists);
		GPOS_ASSERT(!m_fParsingRefExpr);
		GPOS_ASSERT(!m_fParsingAssignExpr);

		m_fParsingAssignExpr = true;
	}
	else
	{
		// parse scalar child
		GPOS_ASSERT(m_fParsingRefExpr || m_fParsingAssignExpr);

		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarArrayRef::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarArrayRef::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRef), element_local_name))
	{
		// add constructed children from child parse handlers
		const ULONG ulSize = this->Length();
		GPOS_ASSERT(3 == ulSize || 4 == ulSize);

		for (ULONG ul = 0; ul < ulSize; ul++)
		{
			CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
			AddChildFromParseHandler(pphChild);
		}

		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRefExpr), element_local_name))
	{
		GPOS_ASSERT(m_fParsingRefExpr);

		m_fParsingRefExpr = false;
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarArrayRefAssignExpr), element_local_name))
	{
		GPOS_ASSERT(m_fParsingAssignExpr);

		m_fParsingAssignExpr = false;
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

// EOF
