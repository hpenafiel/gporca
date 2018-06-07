//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerMDGPDBFunc.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB functions.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDGPDBFunc.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"


using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBFunc::CParseHandlerMDGPDBFunc
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CParseHandlerMDGPDBFunc::CParseHandlerMDGPDBFunc
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname(NULL),
	m_mdid_type_result(NULL),
	m_pdrgpmdidTypes(NULL),
	m_efuncstbl(CMDFunctionGPDB::EfsSentinel)
{}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBFunc::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBFunc::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFunc), element_local_name))
	{
		// parse func name
		const XMLCh *xml_str_func_name = CDXLOperatorFactory::ExtractAttrValue
															(
															attrs,
															EdxltokenName,
															EdxltokenGPDBFunc
															);

		CWStringDynamic *pstrFuncName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xml_str_func_name);
		
		// create a copy of the string in the CMDName constructor
		m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrFuncName);
		
		GPOS_DELETE(pstrFuncName);

		// parse metadata id info
		m_mdid = CDXLOperatorFactory::ExtractConvertAttrValueToMdId
										(
										m_parse_handler_mgr->GetDXLMemoryManager(),
										attrs,
										EdxltokenMdid,
										EdxltokenGPDBFunc
										);
		
		// parse whether func returns a set
		m_fReturnsSet = CDXLOperatorFactory::ExtractConvertAttrValueToBool
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												attrs,
												EdxltokenGPDBFuncReturnsSet,
												EdxltokenGPDBFunc
												);
		// parse whether func is strict
		m_fStrict = CDXLOperatorFactory::ExtractConvertAttrValueToBool
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											attrs,
											EdxltokenGPDBFuncStrict,
											EdxltokenGPDBFunc
											);
		
		// parse func stability property
		const XMLCh *xmlszStbl = CDXLOperatorFactory::ExtractAttrValue
														(
														attrs,
														EdxltokenGPDBFuncStability,
														EdxltokenGPDBFunc
														);
		
		m_efuncstbl = EFuncStability(xmlszStbl);

		// parse func data access property
		const XMLCh *xmlszDataAcc = CDXLOperatorFactory::ExtractAttrValue
														(
														attrs,
														EdxltokenGPDBFuncDataAccess,
														EdxltokenGPDBFunc
														);

		m_efuncdataacc = EFuncDataAccess(xmlszDataAcc);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncResultTypeId), element_local_name))
	{
		// parse result type
		GPOS_ASSERT(NULL != m_mdname);

		m_mdid_type_result = CDXLOperatorFactory::ExtractConvertAttrValueToMdId
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													attrs,
													EdxltokenMdid,
													EdxltokenGPDBFuncResultTypeId
													);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOutputCols), element_local_name))
	{
		// parse output column type
		GPOS_ASSERT(NULL != m_mdname);
		GPOS_ASSERT(NULL == m_pdrgpmdidTypes);

		const XMLCh *xmlszTypes = CDXLOperatorFactory::ExtractAttrValue
															(
															attrs,
															EdxltokenTypeIds,
															EdxltokenOutputCols
															);

		m_pdrgpmdidTypes = CDXLOperatorFactory::ExtractConvertMdIdsToArray
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													xmlszTypes,
													EdxltokenTypeIds,
													EdxltokenOutputCols
													);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBFunc::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBFunc::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFunc), element_local_name))
	{
		// construct the MD func object from its part
		GPOS_ASSERT(m_mdid->IsValid() && NULL != m_mdname);
		
		m_imd_obj = GPOS_NEW(m_memory_pool) CMDFunctionGPDB(m_memory_pool,
												m_mdid,
												m_mdname,
												m_mdid_type_result,
												m_pdrgpmdidTypes,
												m_fReturnsSet,
												m_efuncstbl,
												m_efuncdataacc,
												m_fStrict);
		
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();

	}
	else if (0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncResultTypeId), element_local_name) &&
			0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOutputCols), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBFunc::EFuncStability
//
//	@doc:
//		Parses function stability property from XML string
//
//---------------------------------------------------------------------------
CMDFunctionGPDB::EFuncStbl 
CParseHandlerMDGPDBFunc::EFuncStability
	(
	const XMLCh *xmlsz
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncStable), xmlsz))
	{
		return CMDFunctionGPDB::EfsStable;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncImmutable), xmlsz))
	{
		return CMDFunctionGPDB::EfsImmutable;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncVolatile), xmlsz))
	{
		return CMDFunctionGPDB::EfsVolatile;
	}

	GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncStability)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFunc)->GetBuffer()
		);

	return CMDFunctionGPDB::EfsSentinel;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBFunc::EFuncDataAccess
//
//	@doc:
//		Parses function data access property from XML string
//
//---------------------------------------------------------------------------
CMDFunctionGPDB::EFuncDataAcc
CParseHandlerMDGPDBFunc::EFuncDataAccess
	(
	const XMLCh *xmlsz
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncNoSQL), xmlsz))
	{
		return CMDFunctionGPDB::EfdaNoSQL;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncContainsSQL), xmlsz))
	{
		return CMDFunctionGPDB::EfdaContainsSQL;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncReadsSQLData), xmlsz))
	{
		return CMDFunctionGPDB::EfdaReadsSQLData;
	}

	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBFuncModifiesSQLData), xmlsz))
	{
		return CMDFunctionGPDB::EfdaModifiesSQLData;
	}

	GPOS_RAISE
		(
		gpdxl::ExmaDXL,
		gpdxl::ExmiDXLInvalidAttributeValue,
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFuncDataAccess)->GetBuffer(),
		CDXLTokens::GetDXLTokenStr(EdxltokenGPDBFunc)->GetBuffer()
		);

	return CMDFunctionGPDB::EfdaSentinel;
}

// EOF
