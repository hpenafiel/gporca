//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerScalarLimitCount.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing LimitCount
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarLimitCount.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarLimitCount::CParseHandlerScalarLimitCount
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarLimitCount::CParseHandlerScalarLimitCount
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, pphRoot)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarLimitCount::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarLimitCount::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarLimitCount), element_local_name))
	{
		// parse and create scalar limit count
		CDXLScalarLimitCount *pdxlop = (CDXLScalarLimitCount*) CDXLOperatorFactory::PdxlopLimitCount(m_parse_handler_mgr->Pmm(), attrs);
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode (m_memory_pool,pdxlop);
	}
	else
	{
		// we must have seen a LIMITCOUNT already and initialized its corresponding node
		if (NULL == m_pdxln)
		{
			CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
		}
		// install a scalar element parser for parsing the limit count element
		CParseHandlerBase *pphChild = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		// store parse handler
		this->Append(pphChild);

		pphChild->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarLimitCount::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarLimitCount::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarLimitCount), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	if (0 < ulSize)
	{
		GPOS_ASSERT(1 == ulSize);
		// limit count node was not empty
		CParseHandlerScalarOp *pphChild = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);

		AddChildFromParseHandler(pphChild);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
