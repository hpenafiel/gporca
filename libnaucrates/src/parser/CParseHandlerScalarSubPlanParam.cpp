//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerScalarSubPlanParam.cpp
//
//	@doc:
//		
//		Implementation of the SAX parse handler class for parsing a Param of
//		a scalar SubPlan
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerPhysicalOp.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/dxl/parser/CParseHandlerScalarSubPlanParam.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParam::CParseHandlerScalarSubPlanParam
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlanParam::CParseHandlerScalarSubPlanParam
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerScalarOp(pmp, parse_handler_mgr, pphRoot),
	m_pdxlcr(NULL)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParam::~CParseHandlerScalarSubPlanParam
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CParseHandlerScalarSubPlanParam::~CParseHandlerScalarSubPlanParam()
{
	m_pdxlcr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParam::StartElement
//
//	@doc:
//		Processes a Xerces start element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlanParam::StartElement
	(
	const XMLCh* const, // xmlszUri,
	const XMLCh* const xmlszLocalname,
	const XMLCh* const, // xmlszQname,
	const Attributes &attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParam), xmlszLocalname))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlszLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	m_pdxlcr = CDXLOperatorFactory::Pdxlcr(m_pphm->Pmm(), attrs, EdxltokenScalarSubPlanParam);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerScalarSubPlanParam::EndElement
//
//	@doc:
//		Processes a Xerces end element event
//
//---------------------------------------------------------------------------
void
CParseHandlerScalarSubPlanParam::EndElement
	(
	const XMLCh* const, // xmlszUri,
	const XMLCh* const xmlszLocalname,
	const XMLCh* const // xmlszQname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenScalarSubPlanParam), xmlszLocalname) && NULL != m_pdxln)
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_pphm->Pmm(), xmlszLocalname);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag,	pstr->GetBuffer());
	}

	// deactivate handler
	m_pphm->DeactivateHandler();
}

// EOF
