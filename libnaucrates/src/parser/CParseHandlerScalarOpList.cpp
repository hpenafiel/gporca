//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerScalarOpList.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing scalar
//		operator lists
//	@owner:
//		
//
//	@test:
//
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOpList.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/operators/CDXLScalarOpList.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpList::CParseHandlerScalarOpList
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarOpList::CParseHandlerScalarOpList
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_edxloplisttype(CDXLScalarOpList::EdxloplistSentinel)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpList::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarOpList::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	CDXLScalarOpList::EdxlOpListType edxloplisttype = Edxloplisttype(element_local_name);
	if (NULL == m_pdxln && CDXLScalarOpList::EdxloplistSentinel > edxloplisttype)
	{
		// create the list
		m_edxloplisttype = edxloplisttype;
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode (m_memory_pool, GPOS_NEW(m_memory_pool) CDXLScalarOpList(m_memory_pool, m_edxloplisttype));
	}
	else
	{
		// we must have already initialized the list node
		GPOS_ASSERT(NULL != m_pdxln);

		// parse scalar child
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpList::Edxloplisttype
//
//	@doc:
//		Return the op list type corresponding to the given operator name
//
//---------------------------------------------------------------------------
CDXLScalarOpList::EdxlOpListType
CParseHandlerScalarOpList::Edxloplisttype
	(
	const XMLCh* const element_local_name
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarOpList), element_local_name))
	{
		return CDXLScalarOpList::EdxloplistGeneral;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartLevelEqFilterList), element_local_name))
	{
		return CDXLScalarOpList::EdxloplistEqFilterList;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenPartLevelFilterList), element_local_name))
	{
		return CDXLScalarOpList::EdxloplistFilterList;
	}

	return CDXLScalarOpList::EdxloplistSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarOpList::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarOpList::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	CDXLScalarOpList::EdxlOpListType edxloplisttype = Edxloplisttype(element_local_name);
	if (m_edxloplisttype != edxloplisttype)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// add constructed children from child parse handlers
	const ULONG ulSize = this->Length();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(pphChild);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
