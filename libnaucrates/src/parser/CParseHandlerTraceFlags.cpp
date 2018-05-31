//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CParseHandlerTraceFlags.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing trace flags
//---------------------------------------------------------------------------

#include "gpos/common/CBitSet.h"

#include "naucrates/dxl/parser/CParseHandlerTraceFlags.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"
#include "naucrates/traceflags/traceflags.h"

#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTraceFlags::CParseHandlerTraceFlags
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerTraceFlags::CParseHandlerTraceFlags
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pbs(NULL)
{
	m_pbs = GPOS_NEW(memory_pool) CBitSet(memory_pool, EopttraceSentinel);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTraceFlags::~CParseHandlerTraceFlags
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerTraceFlags::~CParseHandlerTraceFlags()
{
	m_pbs->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTraceFlags::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerTraceFlags::StartElement
	(
	const XMLCh* const , //element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const , //element_qname,
	const Attributes& attrs
	)
{	
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenTraceFlags), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse and tokenize traceflags
	const XMLCh *xmlszTraceFlags = CDXLOperatorFactory::XmlstrFromAttrs
															(
															attrs,
															EdxltokenValue,
															EdxltokenTraceFlags
															);
	
	ULongPtrArray *pdrgpul = CDXLOperatorFactory::PdrgpulFromXMLCh
												(
												m_parse_handler_mgr->Pmm(),
												xmlszTraceFlags, 
												EdxltokenDistrColumns,
												EdxltokenRelation
												);
	
	for (ULONG ul = 0; ul < pdrgpul->Size(); ul++)
	{
		ULONG *pul = (*pdrgpul)[ul];
		m_pbs->ExchangeSet(*pul);
	}
	
	pdrgpul->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTraceFlags::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerTraceFlags::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenTraceFlags), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE( gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTraceFlags::GetParseHandlerType
//
//	@doc:
//		Return the type of the parse handler.
//
//---------------------------------------------------------------------------
EDxlParseHandlerType
CParseHandlerTraceFlags::GetParseHandlerType() const
{
	return EdxlphTraceFlags;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerTraceFlags::Pbs
//
//	@doc:
//		Returns the bitset for the trace flags
//
//---------------------------------------------------------------------------
CBitSet *
CParseHandlerTraceFlags::Pbs()
{
	return m_pbs;
}
// EOF
