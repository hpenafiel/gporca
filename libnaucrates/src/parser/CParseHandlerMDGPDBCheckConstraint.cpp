//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerMDGPDBCheckConstraint.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing an MD check constraint
//
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/md/CMDCheckConstraintGPDB.h"

#include "naucrates/dxl/parser/CParseHandlerMDGPDBCheckConstraint.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBCheckConstraint::CParseHandlerMDGPDBCheckConstraint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDGPDBCheckConstraint::CParseHandlerMDGPDBCheckConstraint
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname(NULL),
	m_pmdidRel(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBCheckConstraint::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBCheckConstraint::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname,
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCheckConstraint), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// new md object
	GPOS_ASSERT(NULL == m_mdid);

	// parse mdid
	m_mdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenMdid, EdxltokenCheckConstraint);

	// parse check constraint name
	const XMLCh *parsed_column_name = CDXLOperatorFactory::XmlstrFromAttrs(attrs, EdxltokenName, EdxltokenCheckConstraint);
	CWStringDynamic *column_name = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), parsed_column_name);

	// create a copy of the string in the CMDName constructor
	m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, column_name);
	GPOS_DELETE(column_name);

	// parse mdid of relation
	m_pmdidRel = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->Pmm(), attrs, EdxltokenRelationMdid, EdxltokenCheckConstraint);

	// create and activate the parse handler for the child scalar expression node
	CParseHandlerBase *pph = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
	m_parse_handler_mgr->ActivateParseHandler(pph);

	// store parse handler
	this->Append(pph);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBCheckConstraint::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBCheckConstraint::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenCheckConstraint), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// get node for default value expression from child parse handler
	CParseHandlerScalarOp *pphOp = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);

	CDXLNode *pdxlnScExpr = pphOp->Pdxln();
	GPOS_ASSERT(NULL != pdxlnScExpr);
	pdxlnScExpr->AddRef();

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDCheckConstraintGPDB(m_memory_pool, m_mdid, m_mdname, m_pmdidRel, pdxlnScExpr);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
