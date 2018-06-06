//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CParseHandlerMDArrayCoerceCast.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB array coerce cast functions
//---------------------------------------------------------------------------

#include "naucrates/md/CMDArrayCoerceCastGPDB.h"

#include "naucrates/dxl/parser/CParseHandlerMDArrayCoerceCast.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

// ctor
CParseHandlerMDArrayCoerceCast::CParseHandlerMDArrayCoerceCast
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root)
{}

// invoked by Xerces to process an opening tag
void
CParseHandlerMDArrayCoerceCast::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBArrayCoerceCast), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// parse func name
	const XMLCh *xmlszFuncName = CDXLOperatorFactory::ExtractAttrValue
														(
														attrs,
														EdxltokenName,
														EdxltokenGPDBArrayCoerceCast
														);

	CMDName *mdname = CDXLUtils::CreateMDNameFromXMLChar(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszFuncName);

	// parse cast properties
	IMDId *pmdid = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->GetDXLMemoryManager(),
									attrs,
									EdxltokenMdid,
									EdxltokenGPDBArrayCoerceCast
									);

	IMDId *pmdidSrc = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->GetDXLMemoryManager(),
									attrs,
									EdxltokenGPDBCastSrcType,
									EdxltokenGPDBArrayCoerceCast
									);

	IMDId *pmdidDest = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->GetDXLMemoryManager(),
									attrs,
									EdxltokenGPDBCastDestType,
									EdxltokenGPDBArrayCoerceCast
									);

	IMDId *pmdidCastFunc = CDXLOperatorFactory::PmdidFromAttrs
									(
									m_parse_handler_mgr->GetDXLMemoryManager(),
									attrs,
									EdxltokenGPDBCastFuncId,
									EdxltokenGPDBArrayCoerceCast
									);

	// parse whether func returns a set
	BOOL fBinaryCoercible = CDXLOperatorFactory::ExtractConvertAttrValueToBool
									(
									m_parse_handler_mgr->GetDXLMemoryManager(),
									attrs,
									EdxltokenGPDBCastBinaryCoercible,
									EdxltokenGPDBArrayCoerceCast
									);

	// parse coercion path type
	IMDCast::EmdCoercepathType eCoercePathType = (IMDCast::EmdCoercepathType)
													CDXLOperatorFactory::ExtractConvertAttrValueToInt
															(
															m_parse_handler_mgr->GetDXLMemoryManager(),
															attrs,
															EdxltokenGPDBCastCoercePathType,
															EdxltokenGPDBArrayCoerceCast
															);

	INT type_modifier = CDXLOperatorFactory::ExtractConvertAttrValueToInt
							(
							m_parse_handler_mgr->GetDXLMemoryManager(),
							attrs,
							EdxltokenTypeMod,
							EdxltokenGPDBArrayCoerceCast,
							true,
							IDefaultTypeModifier
							);

	BOOL fIsExplicit =CDXLOperatorFactory::ExtractConvertAttrValueToBool
									(
									m_parse_handler_mgr->GetDXLMemoryManager(),
									attrs,
									EdxltokenIsExplicit,
									EdxltokenGPDBArrayCoerceCast
									);

	EdxlCoercionForm edcf = (EdxlCoercionForm) CDXLOperatorFactory::ExtractConvertAttrValueToInt
																		(
																		m_parse_handler_mgr->GetDXLMemoryManager(),
																		attrs,
																		EdxltokenCoercionForm,
																		EdxltokenGPDBArrayCoerceCast
																		);

	INT iLoc = CDXLOperatorFactory::ExtractConvertAttrValueToInt
							(
							m_parse_handler_mgr->GetDXLMemoryManager(),
							attrs,
							EdxltokenLocation,
							EdxltokenGPDBArrayCoerceCast
							);

	m_imd_obj = GPOS_NEW(m_memory_pool) CMDArrayCoerceCastGPDB(m_memory_pool, pmdid, mdname, pmdidSrc, pmdidDest, fBinaryCoercible, pmdidCastFunc, eCoercePathType, type_modifier, fIsExplicit, edcf, iLoc);
}

// invoked by Xerces to process a closing tag
void
CParseHandlerMDArrayCoerceCast::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBArrayCoerceCast), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}

// EOF
