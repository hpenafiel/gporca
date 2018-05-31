//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CParseHandlerMDCast.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB cast functions
//---------------------------------------------------------------------------

#include "naucrates/md/CMDCastGPDB.h"

#include "naucrates/dxl/parser/CParseHandlerMDCast.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDCast::CParseHandlerMDCast
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDCast::CParseHandlerMDCast
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root)
{}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDCast::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDCast::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBCast), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
	
	// parse func name
	const XMLCh *xmlszFuncName = CDXLOperatorFactory::XmlstrFromAttrs
														(
														attrs,
														EdxltokenName,
														EdxltokenGPDBCast
														);

	CMDName *mdname = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->Pmm(), xmlszFuncName);


	// parse cast properties
	IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->Pmm(),
									attrs,
									EdxltokenMdid,
									EdxltokenGPDBCast
									);
	
	IMDId *pmdidSrc = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->Pmm(),
									attrs,
									EdxltokenGPDBCastSrcType,
									EdxltokenGPDBCast
									);
	
	IMDId *pmdidDest = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->Pmm(),
									attrs,
									EdxltokenGPDBCastDestType,
									EdxltokenGPDBCast
									);
	
	IMDId *pmdidCastFunc = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->Pmm(),
									attrs,
									EdxltokenGPDBCastFuncId,
									EdxltokenGPDBCast
									);

	// parse whether func returns a set
	BOOL fBinaryCoercible = CDXLOperatorFactory::FValueFromAttrs
											(
											m_parse_handler_mgr->Pmm(),
											attrs,
											EdxltokenGPDBCastBinaryCoercible,
											EdxltokenGPDBCast
											);

	IMDCast::EmdCoercepathType eCoercePathType = (IMDCast::EmdCoercepathType)
													CDXLOperatorFactory::IValueFromAttrs
															(
															m_parse_handler_mgr->Pmm(),
															attrs,
															EdxltokenGPDBCastCoercePathType,
															EdxltokenGPDBCast,
															true		// eCoercePathType is optional
															);

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDCastGPDB(m_memory_pool, pmdid, mdname, pmdidSrc, pmdidDest, fBinaryCoercible, pmdidCastFunc, eCoercePathType);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDCast::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDCast::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBCast), element_local_name))
	{
		CWStringDynamic *pstr = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->Pmm(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, pstr->GetBuffer());
	}
		
	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
