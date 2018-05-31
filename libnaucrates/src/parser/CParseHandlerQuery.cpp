//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerQuery.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing queries.
//		
//
//	@owner: 
//		
//
//	@test:
//private:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerQuery.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerLogicalOp.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerQueryOutput.h"
#include "naucrates/dxl/parser/CParseHandlerCTEList.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::CParseHandlerQuery
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerQuery::CParseHandlerQuery
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pdxln(NULL),
	m_output_colums_dxl_array(NULL),
	m_cte_producer_dxl_array(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::~CParseHandlerQuery
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerQuery::~CParseHandlerQuery()
{
	CRefCount::SafeRelease(m_pdxln);
	CRefCount::SafeRelease(m_output_colums_dxl_array);
	CRefCount::SafeRelease(m_cte_producer_dxl_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::Pdxln
//
//	@doc:
//		Root of constructed DXL plan
//
//---------------------------------------------------------------------------
CDXLNode *
CParseHandlerQuery::Pdxln() const
{
	return m_pdxln;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::GetOutputColumnsDXLArray
//
//	@doc:
//		Returns the list of query output columns
//
//---------------------------------------------------------------------------
DrgPdxln *
CParseHandlerQuery::GetOutputColumnsDXLArray() const
{
	return m_output_colums_dxl_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::GetCTEProducerDXLArray
//
//	@doc:
//		Returns the list of CTEs
//
//---------------------------------------------------------------------------
DrgPdxln *
CParseHandlerQuery::GetCTEProducerDXLArray() const
{
	return m_cte_producer_dxl_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::GetParseHandlerType
//
//	@doc:
//		Parse handler type
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerQuery::GetParseHandlerType() const
{
	return EdxlphQuery;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerQuery::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& // attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenQuery), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL != m_memory_pool);

	// create parse handler for the query output node
	CParseHandlerBase *pphQueryOutput = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenQueryOutput), m_parse_handler_mgr, this);

	// create parse handler for the CTE list
	CParseHandlerBase *pphCTE = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCTEList), m_parse_handler_mgr, this);

	// create a parse handler for logical nodes
	CParseHandlerBase *pph = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);

	m_parse_handler_mgr->ActivateParseHandler(pph);
	m_parse_handler_mgr->ActivateParseHandler(pphCTE);
	m_parse_handler_mgr->ActivateParseHandler(pphQueryOutput);

	// store parse handlers
	this->Append(pphQueryOutput);
	this->Append(pphCTE);
	this->Append(pph);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerQuery::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenQuery), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerQueryOutput *pphQueryOutput = dynamic_cast<CParseHandlerQueryOutput *>((*this)[0]);
	GPOS_ASSERT(NULL != pphQueryOutput && NULL != pphQueryOutput->GetOutputColumnsDXLArray());

	// store constructed node
	m_output_colums_dxl_array = pphQueryOutput->GetOutputColumnsDXLArray();
	m_output_colums_dxl_array->AddRef();

	CParseHandlerCTEList *pphCTE = dynamic_cast<CParseHandlerCTEList *>((*this)[1]);
	GPOS_ASSERT(NULL != pphCTE && NULL != pphCTE->Pdrgpdxln());

	m_cte_producer_dxl_array = pphCTE->Pdrgpdxln();
	m_cte_producer_dxl_array->AddRef();

	CParseHandlerLogicalOp *pphLgOp = dynamic_cast<CParseHandlerLogicalOp *>((*this)[2]);
	GPOS_ASSERT(NULL != pphLgOp && NULL != pphLgOp->Pdxln());

	// store constructed node
	m_pdxln = pphLgOp->Pdxln();
	m_pdxln->AddRef();

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF

