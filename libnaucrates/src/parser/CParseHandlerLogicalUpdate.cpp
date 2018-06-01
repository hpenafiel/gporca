//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalUpdate.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		update operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalUpdate.h"

#include "naucrates/dxl/parser/CParseHandlerTableDescr.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalUpdate::CParseHandlerLogicalUpdate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalUpdate::CParseHandlerLogicalUpdate
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerLogicalOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_ulCtid(0),
	m_ulSegmentId(0),
	m_pdrgpulDelete(NULL),
	m_pdrgpulInsert(NULL),
	m_fPreserveOids(false),
	m_ulTupleOidColId(0)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalUpdate::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalUpdate::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalUpdate), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	m_ulCtid = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenCtidColId, EdxltokenLogicalUpdate);
	m_ulSegmentId = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenGpSegmentIdColId, EdxltokenLogicalUpdate);

	const XMLCh *xmlszDeleteColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenDeleteCols, EdxltokenLogicalUpdate);
	m_pdrgpulDelete = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszDeleteColIds, EdxltokenDeleteCols, EdxltokenLogicalUpdate);

	const XMLCh *xmlszInsertColIds = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenInsertCols, EdxltokenLogicalUpdate);
	m_pdrgpulInsert = CDXLOperatorFactory::PdrgpulFromXMLCh(m_parse_handler_mgr->Pmm(), xmlszInsertColIds, EdxltokenInsertCols, EdxltokenLogicalUpdate);

	const XMLCh *xmlszPreserveOids = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenUpdatePreservesOids));
	if (NULL != xmlszPreserveOids)
	{
		m_fPreserveOids = CDXLOperatorFactory::FValueFromXmlstr
											(
											m_parse_handler_mgr->Pmm(),
											xmlszPreserveOids,
											EdxltokenUpdatePreservesOids,
											EdxltokenLogicalUpdate
											);
	}
	
	if (m_fPreserveOids)
	{
		m_ulTupleOidColId = CDXLOperatorFactory::UlValueFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenTupleOidColId, EdxltokenLogicalUpdate);
	}

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
//		CParseHandlerLogicalUpdate::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalUpdate::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalUpdate), element_local_name))
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
							GPOS_NEW(m_memory_pool) CDXLLogicalUpdate(m_memory_pool, pdxltabdesc, m_ulCtid, m_ulSegmentId, m_pdrgpulDelete, m_pdrgpulInsert, m_fPreserveOids, m_ulTupleOidColId)
							);

	AddChildFromParseHandler(pphChild);

#ifdef GPOS_DEBUG
	m_pdxln->GetOperator()->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
