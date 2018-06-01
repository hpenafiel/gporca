//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalDelete.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		delete operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalDelete.h"

#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalDelete::CParseHandlerLogicalDelete
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalDelete::CParseHandlerLogicalDelete
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalDelete::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalDelete::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalDelete), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	m_ulCtid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCtidColId, EdxltokenLogicalDelete);
	m_ulSegmentId = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenGpSegmentIdColId, EdxltokenLogicalDelete);

	const XMLCh *xmlszDeleteColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDeleteCols, EdxltokenLogicalDelete);
	m_pdrgpulDelete = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDeleteColIds, EdxltokenDeleteCols, EdxltokenLogicalDelete);

	// parse handler for logical operator
	CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphChild);

	//parse handler for the table descriptor
	CParseHandlerBase *pphTabDesc = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenTableDescr), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pphTabDesc);

	// store child parse handler in array
	this->Append(pphTabDesc);
	this->Append(pphChild);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalDelete::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalDelete::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalDelete), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(2 == this->Length());

	CParseHandlerTableDescr *pphTabDesc = dynamic_cast<CParseHandlerTableDescr*>((*this)[0]);
	CParseHandlerLogicalOp *pphChild = dynamic_cast<CParseHandlerLogicalOp*>((*this)[1]);

	GPOS_ASSERT(NULL != pphTabDesc->Pdxltabdesc());
	GPOS_ASSERT(NULL != pphChild->Pdxln());

	CDXLTableDescr *pdxltabdesc = pphTabDesc->Pdxltabdesc();
	pdxltabdesc->AddRef();

	m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode
							(
							m_memory_pool,
							GPOS_NEW(m_memory_pool) CDXLLogicalDelete(m_memory_pool, pdxltabdesc, m_ulCtid, m_ulSegmentId, m_pdrgpulDelete)
							);

	AddChildFromParseHandler(pphChild);

#ifdef GPOS_DEBUG
	m_pdxln->GetOperator()->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
