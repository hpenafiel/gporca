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
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerBase(pmp, parse_handler_mgr, pphRoot),
	m_pdxln(NULL),
	m_pdrgpdxlnOutputCols(NULL),
	m_pdrgpdxlnCTE(NULL)
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
	CRefCount::SafeRelease(m_pdrgpdxlnOutputCols);
	CRefCount::SafeRelease(m_pdrgpdxlnCTE);
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
//		CParseHandlerQuery::PdrgpdxlnOutputCols
//
//	@doc:
//		Returns the list of query output columns
//
//---------------------------------------------------------------------------
DrgPdxln *
CParseHandlerQuery::PdrgpdxlnOutputCols() const
{
	return m_pdrgpdxlnOutputCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::PdrgpdxlnCTE
//
//	@doc:
//		Returns the list of CTEs
//
//---------------------------------------------------------------------------
DrgPdxln *
CParseHandlerQuery::PdrgpdxlnCTE() const
{
	return m_pdrgpdxlnCTE;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerQuery::Edxlphtype
//
//	@doc:
//		Parse handler type
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerQuery::Edxlphtype() const
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
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	GPOS_ASSERT(NULL != m_memory_pool);

	// create parse handler for the query output node
	CParseHandlerBase *pphQueryOutput = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenQueryOutput), m_pphm, this);

	// create parse handler for the CTE list
	CParseHandlerBase *pphCTE = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenCTEList), m_pphm, this);

	// create a parse handler for logical nodes
	CParseHandlerBase *pph = CParseHandlerFactory::Pph(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_pphm, this);

	m_pphm->ActivateParseHandler(pph);
	m_pphm->ActivateParseHandler(pphCTE);
	m_pphm->ActivateParseHandler(pphQueryOutput);

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
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	CParseHandlerQueryOutput *pphQueryOutput = dynamic_cast<CParseHandlerQueryOutput *>((*this)[0]);
	GPOS_ASSERT(NULL != pphQueryOutput && NULL != pphQueryOutput->PdrgpdxlnOutputCols());

	// store constructed node
	m_pdrgpdxlnOutputCols = pphQueryOutput->PdrgpdxlnOutputCols();
	m_pdrgpdxlnOutputCols->AddRef();

	CParseHandlerCTEList *pphCTE = dynamic_cast<CParseHandlerCTEList *>((*this)[1]);
	GPOS_ASSERT(NULL != pphCTE && NULL != pphCTE->Pdrgpdxln());

	m_pdrgpdxlnCTE = pphCTE->Pdrgpdxln();
	m_pdrgpdxlnCTE->AddRef();

	CParseHandlerLogicalOp *pphLgOp = dynamic_cast<CParseHandlerLogicalOp *>((*this)[2]);
	GPOS_ASSERT(NULL != pphLgOp && NULL != pphLgOp->Pdxln());

	// store constructed node
	m_pdxln = pphLgOp->Pdxln();
	m_pdxln->AddRef();

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF

