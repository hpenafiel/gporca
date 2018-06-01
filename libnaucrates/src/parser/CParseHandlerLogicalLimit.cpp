//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerLogicalLimit.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical
//		limit operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalLimit.h"
#include "naucrates/dxl/parser/CParseHandlerSortColList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalLimit::CParseHandlerLogicalLimit
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalLimit::CParseHandlerLogicalLimit
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
//		CParseHandlerLogicalLimit::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalLimit::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalLimit), element_local_name))
	{
		const XMLCh *xmlszNonRemovableLimit = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenTopLimitUnderDML));
		BOOL fKeepLimit = false;
		if (xmlszNonRemovableLimit)
		{
			fKeepLimit = CDXLOperatorFactory::FValueFromXmlstr
										(
										m_parse_handler_mgr->Pmm(),
										xmlszNonRemovableLimit,
										EdxltokenTopLimitUnderDML,
										EdxltokenLogicalLimit
										);
		}

		m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, GPOS_NEW(m_memory_pool) CDXLLogicalLimit(m_memory_pool, fKeepLimit));

		// create child node parsers

		// parse handler for logical operator
		CParseHandlerBase *pphChild = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenLogical), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphChild);

		CParseHandlerBase *pphOffset = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarLimitOffset), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOffset);

		CParseHandlerBase *pphCount = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarLimitCount), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphCount);

		// parse handler for the sorting column list
		CParseHandlerBase *pphSortColList = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarSortColList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphSortColList);

		// store child parse handler in array
		this->Append(pphSortColList);
		this->Append(pphCount);
		this->Append(pphOffset);
		this->Append(pphChild);
	}
	else
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalLimit::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalLimit::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenLogicalLimit), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	GPOS_ASSERT(NULL != m_pdxln );
	GPOS_ASSERT(4 == this->Length());

	CParseHandlerSortColList *pphSortColList = dynamic_cast<CParseHandlerSortColList*>((*this)[0]);
	CParseHandlerScalarOp *pphCount = dynamic_cast<CParseHandlerScalarOp *>((*this)[1]);
	CParseHandlerScalarOp *pphOffSet = dynamic_cast<CParseHandlerScalarOp *>((*this)[2]);
	CParseHandlerLogicalOp *pphChild = dynamic_cast<CParseHandlerLogicalOp*>((*this)[3]);

	GPOS_ASSERT(NULL != pphChild->Pdxln());

	AddChildFromParseHandler(pphSortColList);
	AddChildFromParseHandler(pphCount);
	AddChildFromParseHandler(pphOffSet);
	AddChildFromParseHandler(pphChild);

#ifdef GPOS_DEBUG
	m_pdxln->GetOperator()->AssertValid(m_pdxln, false /* validate_children */);
#endif // GPOS_DEBUG

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}
// EOF
