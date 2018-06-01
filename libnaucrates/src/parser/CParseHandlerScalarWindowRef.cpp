//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarWindowRef.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar WindowRef
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerScalarWindowRef.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarWindowRef::CParseHandlerScalarWindowRef
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarWindowRef::CParseHandlerScalarWindowRef
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarWindowRef::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarWindowRef::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarWindowref), element_local_name))
	{
		// parse and create scalar WindowRef (window function)
		CDXLScalarWindowRef *dxl_op =
				(CDXLScalarWindowRef*) CDXLOperatorFactory::PdxlopWindowRef(m_parse_handler_mgr->Pmm(), attrs);

		// construct node from the created scalar window ref
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, dxl_op);
	}
	else
	{
		// we must have seen an window ref already and initialized the window ref node
		GPOS_ASSERT(NULL != m_pdxln);

		CParseHandlerBase *pphOp =
				CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOp);

		// store parse handlers
		this->Append(pphOp);

		pphOp->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarWindowRef::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarWindowRef::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarWindowref), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	const ULONG ulSize = this->Length();
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerScalarOp *pphOp = dynamic_cast<CParseHandlerScalarOp *>((*this)[ul]);
		AddChildFromParseHandler(pphOp);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
