//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerScalarBoolExpr.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing scalar BoolExpr.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarBoolExpr.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBoolExpr::CParseHandlerScalarBoolExpr
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerScalarBoolExpr::CParseHandlerScalarBoolExpr
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerScalarOp(memory_pool, parse_handler_mgr, parse_handler_root),
	m_edxlBoolType(Edxland)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBoolExpr::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBoolExpr::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if ((0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolAnd), element_local_name)) ||
		(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolOr), element_local_name)) ||
		(0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolNot), element_local_name)))
	{
		if (NULL == m_pdxln)
		{
			if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolNot), element_local_name))
			{
				m_edxlBoolType = Edxlnot;
			}
			else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolOr), element_local_name))
			{
				m_edxlBoolType = Edxlor;
			}

			// parse and create scalar BoolExpr
			CDXLScalarBoolExpr *pdxlop = (CDXLScalarBoolExpr*) CDXLOperatorFactory::PdxlopBoolExpr(m_parse_handler_mgr->Pmm(), m_edxlBoolType);

			// construct node from the created child nodes
			m_pdxln = GPOS_NEW(m_memory_pool) CDXLNode(m_memory_pool, pdxlop);
		}
		else
		{

			// This is to support nested BoolExpr. TODO:  - create a separate xml tag for boolean expression
			CParseHandlerBase *pphBoolExpr = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalarBoolOr), m_parse_handler_mgr, this);
			m_parse_handler_mgr->ActivateParseHandler(pphBoolExpr);

			// store parse handlers
			this->Append(pphBoolExpr);

			pphBoolExpr->startElement(element_uri, element_local_name, element_qname, attrs);
		}
	}
	else
	{
		if(NULL == m_pdxln)
		{
			GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name)->GetBuffer());
		}

		CParseHandlerBase *pphOp = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenScalar), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOp);

		// store parse handlers
		this->Append(pphOp);

		pphOp->startElement(element_uri, element_local_name, element_qname, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBoolExpr::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarBoolExpr::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	EdxlBoolExprType edxlBoolType =
			CParseHandlerScalarBoolExpr::EdxlBoolType(element_local_name);

	if(EdxlBoolExprTypeSentinel == edxlBoolType || m_edxlBoolType != edxlBoolType)
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name)->GetBuffer());
	}

	const ULONG ulSize = this->Length();
	// If the operation is NOT then it only has one child.
	if (
	    ((((CDXLScalarBoolExpr*) m_pdxln->Pdxlop())->EdxlBoolType() == Edxlnot)
		&& (1 != ulSize))
		||
		((((CDXLScalarBoolExpr*) m_pdxln->Pdxlop())->EdxlBoolType() != Edxlnot)
		&& (2 > ulSize))
	  )
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLIncorrectNumberOfChildren, CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name)->GetBuffer());
	}

	// add constructed children from child parse handlers
	for (ULONG ul = 0; ul < ulSize; ul++)
	{
		CParseHandlerScalarOp *pph = dynamic_cast<CParseHandlerScalarOp*>((*this)[ul]);
		AddChildFromParseHandler(pph);
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarBoolExpr::EdxlBoolType
//
//	@doc:
//		Parse the bool type from the attribute value. Raise exception if it is invalid
//
//---------------------------------------------------------------------------
EdxlBoolExprType
CParseHandlerScalarBoolExpr::EdxlBoolType
	(
	const XMLCh *xmlszBoolType
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolNot), xmlszBoolType))
	{
		return Edxlnot;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolAnd), xmlszBoolType))
	{
		return Edxland;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarBoolOr), xmlszBoolType))
	{
		return Edxlor;
	}

	return EdxlBoolExprTypeSentinel;
}

// EOF
