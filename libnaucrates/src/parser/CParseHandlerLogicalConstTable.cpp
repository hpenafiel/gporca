//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalConstTable.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical 
//		const tables.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalConstTable.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerColDescr.h"

#include "naucrates/dxl/operators/CDXLLogicalConstTable.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalConstTable::CParseHandlerLogicalConstTable
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalConstTable::CParseHandlerLogicalConstTable
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pdrgpdrgpdxldatum(NULL),
	m_pdrgpdxldatum(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalConstTable::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalConstTable::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalConstTable), element_local_name))
	{
		// start of a const table operator node
		GPOS_ASSERT(0 == this->Length());
		GPOS_ASSERT(NULL == m_pdrgpdrgpdxldatum);

		// initialize the array of const tuples (datum arrays)
		m_pdrgpdrgpdxldatum = GPOS_NEW(m_memory_pool) DXLDatumArrays(m_memory_pool);

		// install a parse handler for the columns
		CParseHandlerBase *pphColDescr = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenColumns), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphColDescr);
		
		// store parse handler
		this->Append(pphColDescr);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenConstTuple), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pdrgpdrgpdxldatum); // we must have already seen a logical const table
		GPOS_ASSERT(NULL == m_pdrgpdxldatum);

		// initialize the array of datums (const tuple)
		m_pdrgpdxldatum = GPOS_NEW(m_memory_pool) DXLDatumArray(m_memory_pool);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDatum), element_local_name))
	{
		// we must have already seen a logical const table and a const tuple
		GPOS_ASSERT(NULL != m_pdrgpdrgpdxldatum);
		GPOS_ASSERT(NULL != m_pdrgpdxldatum);

		// translate the datum and add it to the datum array
		CDXLDatum *datum_dxl = CDXLOperatorFactory::Pdxldatum(m_parse_handler_mgr->Pmm(), attrs, EdxltokenScalarConstValue);
		m_pdrgpdxldatum->Append(datum_dxl);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalConstTable::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalConstTable::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalConstTable), element_local_name))
	{
		GPOS_ASSERT(1 == this->Length());

		CParseHandlerColDescr *pphColDescr = dynamic_cast<CParseHandlerColDescr *>((*this)[0]);
		GPOS_ASSERT(NULL != pphColDescr->GetColumnDescrDXLArray());

		ColumnDescrDXLArray *pdrgpdxlcd = pphColDescr->GetColumnDescrDXLArray();
		pdrgpdxlcd->AddRef();

		CDXLLogicalConstTable *pdxlopConstTable = GPOS_NEW(m_memory_pool) CDXLLogicalConstTable(m_memory_pool, pdrgpdxlcd, m_pdrgpdrgpdxldatum);
		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlopConstTable);

#ifdef GPOS_DEBUG
	pdxlopConstTable->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

		// deactivate handler
	  	m_parse_handler_mgr->DeactivateHandler();
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenConstTuple), element_local_name))
	{
		GPOS_ASSERT(NULL != m_pdrgpdxldatum);
		m_pdrgpdrgpdxldatum->Append(m_pdrgpdxldatum);

		m_pdrgpdxldatum = NULL; // intialize for the parsing the next const tuple (if needed)
	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenDatum), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}
// EOF
