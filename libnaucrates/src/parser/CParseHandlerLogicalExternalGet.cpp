//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CParseHandlerLogicalExternalGet.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing logical external get
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerLogicalExternalGet.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalExternalGet::CParseHandlerLogicalExternalGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerLogicalExternalGet::CParseHandlerLogicalExternalGet
	(
	IMemoryPool *pmp,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *pphRoot
	)
	:
	CParseHandlerLogicalGet(pmp, parse_handler_mgr, pphRoot)
{}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalExternalGet::StartElement
	(
	const XMLCh* const, // xmlszUri,
	const XMLCh* const xmlszLocalname,
	const XMLCh* const, // xmlszQname
	const Attributes& //attrs
	)
{
	CParseHandlerLogicalGet::StartElement(xmlszLocalname, EdxltokenLogicalExternalGet);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerLogicalGet::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerLogicalExternalGet::EndElement
	(
	const XMLCh* const, // xmlszUri,
	const XMLCh* const xmlszLocalname,
	const XMLCh* const // xmlszQname
	)
{
	CParseHandlerLogicalGet::EndElement(xmlszLocalname, EdxltokenLogicalExternalGet);
}

// EOF
