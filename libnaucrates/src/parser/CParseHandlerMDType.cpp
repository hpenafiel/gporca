//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerMDType.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB types.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDType.h"

#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/md/CMDTypeBoolGPDB.h"
#include "naucrates/md/CMDTypeGenericGPDB.h"
#include "naucrates/md/CMDTypeInt2GPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/CMDTypeOidGPDB.h"

using namespace gpdxl;
using namespace gpmd;

XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::CParseHandlerMDType
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMDType::CParseHandlerMDType
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname(NULL),
	m_pmdidOpEq(NULL),
	m_pmdidOpNEq(NULL),
	m_pmdidOpLT(NULL),
	m_pmdidOpLEq(NULL),
	m_pmdidOpGT(NULL),
	m_pmdidOpGEq(NULL),
	m_pmdidOpComp(NULL),
	m_pmdidMin(NULL),
	m_pmdidMax(NULL),
	m_pmdidAvg(NULL),
	m_pmdidSum(NULL),
	m_pmdidCount(NULL),
	m_fHashable(false),
	m_fComposite(false),
	m_pmdidBaseRelation(NULL),
	m_pmdidTypeArray(NULL)
{
	// default: no aggregates for type
	m_pmdidMin = GPOS_NEW(memory_pool) CMDIdGPDB(0);
	m_pmdidMax = GPOS_NEW(memory_pool) CMDIdGPDB(0);
	m_pmdidAvg = GPOS_NEW(memory_pool) CMDIdGPDB(0);
	m_pmdidSum = GPOS_NEW(memory_pool) CMDIdGPDB(0);
	m_pmdidCount = GPOS_NEW(memory_pool) CMDIdGPDB(0);
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::~CParseHandlerMDType
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerMDType::~CParseHandlerMDType()
{
	m_mdid->Release();
	m_pmdidOpEq->Release();
	m_pmdidOpNEq->Release();
	m_pmdidOpLT->Release();
	m_pmdidOpLEq->Release();
	m_pmdidOpGT->Release();
	m_pmdidOpGEq->Release();
	m_pmdidOpComp->Release();
	m_pmdidTypeArray->Release();
	m_pmdidMin->Release();
	m_pmdidMax->Release();
	m_pmdidAvg->Release();
	m_pmdidSum->Release();
	m_pmdidCount->Release();
	CRefCount::SafeRelease(m_pmdidBaseRelation);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDType::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDType), element_local_name))
	{
		// parse metadata id info
		m_mdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, EdxltokenMDType);
		
		if (!FBuiltInType(m_mdid))
		{
			// parse type name
			const XMLCh *xmlszTypeName = CDXLOperatorFactory::ExtractAttrValue
																(
																attrs,
																EdxltokenName,
																EdxltokenMDType
																);

			CWStringDynamic *pstrTypeName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszTypeName);

			// create a copy of the string in the CMDName constructor
			m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrTypeName);
			GPOS_DELETE(pstrTypeName);
			
			// parse if type is redistributable
			m_fRedistributable = CDXLOperatorFactory::ExtractConvertAttrValueToBool
														(
														m_parse_handler_mgr->GetDXLMemoryManager(),
														attrs,
														EdxltokenMDTypeRedistributable,
														EdxltokenMDType
														);

			// parse if type is passed by value
			m_fByValue = CDXLOperatorFactory::ExtractConvertAttrValueToBool
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												attrs,
												EdxltokenMDTypeByValue,
												EdxltokenMDType
												);
			
			// parse if type is hashable
			m_fHashable = CDXLOperatorFactory::ExtractConvertAttrValueToBool
										(
										m_parse_handler_mgr->GetDXLMemoryManager(),
										attrs,
										EdxltokenMDTypeHashable,
										EdxltokenMDType
										);

			// parse if type is composite
			const XMLCh *xmlszAttributeVal = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenMDTypeComposite));
			if (NULL == xmlszAttributeVal)
			{
				m_fComposite = false;
			}
			else
			{
				m_fComposite = CDXLOperatorFactory::ConvertAttrValueToBool
										(
										m_parse_handler_mgr->GetDXLMemoryManager(),
										xmlszAttributeVal,
										EdxltokenMDTypeComposite,
										EdxltokenMDType
										);
			}

			if (m_fComposite)
			{
				// get base relation id
				m_pmdidBaseRelation = CDXLOperatorFactory::PmdidFromAttrs
										(
										m_parse_handler_mgr->GetDXLMemoryManager(),
										attrs,
										EdxltokenMDTypeRelid,
										EdxltokenMDType
										);
			}

			// parse if type is fixed-length			
			m_fFixedLength = CDXLOperatorFactory::ExtractConvertAttrValueToBool
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											attrs,
											EdxltokenMDTypeFixedLength,
											EdxltokenMDType
											);
			if (m_fFixedLength)
			{
				// get type length
				m_iLength = CDXLOperatorFactory::ExtractConvertAttrValueToInt
											(
											m_parse_handler_mgr->GetDXLMemoryManager(),
											attrs,
											EdxltokenMDTypeLength,
											EdxltokenMDType
											);
			}
		}
	}
	else
	{
		ParseMdid(element_local_name, attrs);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::ParseMdid
//
//	@doc:
//		Parse the mdid
//
//---------------------------------------------------------------------------
void
CParseHandlerMDType::ParseMdid
	(
	const XMLCh *element_local_name, 	
	const Attributes& attrs
	)
{
	const SMdidMapElem rgmdidmap[] =
	{
		{EdxltokenMDTypeEqOp, &m_pmdidOpEq},
		{EdxltokenMDTypeNEqOp, &m_pmdidOpNEq},
		{EdxltokenMDTypeLTOp, &m_pmdidOpLT},
		{EdxltokenMDTypeLEqOp, &m_pmdidOpLEq},
		{EdxltokenMDTypeGTOp, &m_pmdidOpGT},
		{EdxltokenMDTypeGEqOp, &m_pmdidOpGEq},
		{EdxltokenMDTypeCompOp, &m_pmdidOpComp},
		{EdxltokenMDTypeArray, &m_pmdidTypeArray},
		{EdxltokenMDTypeAggMin, &m_pmdidMin},
		{EdxltokenMDTypeAggMax, &m_pmdidMax},
		{EdxltokenMDTypeAggAvg, &m_pmdidAvg},
		{EdxltokenMDTypeAggSum, &m_pmdidSum},
		{EdxltokenMDTypeAggCount, &m_pmdidCount},
	};
	
	Edxltoken token_type = EdxltokenSentinel;
	IMDId **ppmdid = NULL;
	for (ULONG ul = 0; ul < GPOS_ARRAY_SIZE(rgmdidmap); ul++)
	{
		SMdidMapElem mdidmapelem = rgmdidmap[ul];
		if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(mdidmapelem.m_edxltoken), element_local_name))
		{
			token_type = mdidmapelem.m_edxltoken;
			ppmdid = mdidmapelem.m_ppmdid;
		}
	}

	GPOS_ASSERT(EdxltokenSentinel != token_type);
	GPOS_ASSERT(NULL != ppmdid);

	if (m_pmdidMin == *ppmdid || m_pmdidMax == *ppmdid || m_pmdidAvg == *ppmdid  || 
		m_pmdidSum == *ppmdid || m_pmdidCount == *ppmdid)
	{
		(*ppmdid)->Release();
	}
	
	*ppmdid = CDXLOperatorFactory::PmdidFromAttrs(m_parse_handler_mgr->GetDXLMemoryManager(), attrs, EdxltokenMdid, token_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::FBuiltInType
//
//	@doc:
//		Is this a built-in type
//
//---------------------------------------------------------------------------
BOOL
CParseHandlerMDType::FBuiltInType
	(
	const IMDId *pmdid
	)
	const
{
	if (IMDId::EmdidGPDB != pmdid->Emdidt())
	{
		return false;
	}
	
	const CMDIdGPDB *pmdidGPDB = CMDIdGPDB::PmdidConvert(pmdid);
	
	switch (pmdidGPDB->OidObjectId())
	{
		case GPDB_INT2:
		case GPDB_INT4:
		case GPDB_INT8:
		case GPDB_BOOL:
		case GPDB_OID:
			return true;
		default:
			return false;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::Ppmdid
//
//	@doc:
//		Return the address of the mdid variable corresponding to the dxl token
//
//---------------------------------------------------------------------------
IMDId **
CParseHandlerMDType::Ppmdid
	(
	Edxltoken token_type
	)
{
	switch (token_type)
		{
			case EdxltokenMDTypeEqOp:
				return &m_pmdidOpEq;

			case EdxltokenMDTypeNEqOp:
				return &m_pmdidOpNEq;

			case EdxltokenMDTypeLTOp:
				return &m_pmdidOpLT;

			case EdxltokenMDTypeLEqOp:
				return &m_pmdidOpLEq;

			case EdxltokenMDTypeGTOp:
				return &m_pmdidOpGT;

			case EdxltokenMDTypeGEqOp:
				return &m_pmdidOpGEq;

			case EdxltokenMDTypeCompOp:
				return &m_pmdidOpComp;

			case EdxltokenMDTypeArray:
				return &m_pmdidTypeArray;

			default:
				break;
		}
	GPOS_ASSERT(!"Unexpected DXL token when parsing MDType");
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDType::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDType::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenMDType), element_local_name))
	{
		// construct the MD type object from its part
		GPOS_ASSERT(m_mdid->IsValid());

		// TODO:  - Jan 30, 2012; add support for other types of mdids
		
		const CMDIdGPDB *pmdidGPDB = CMDIdGPDB::PmdidConvert(m_mdid);
		
		switch(pmdidGPDB->OidObjectId())
		{
			case GPDB_INT2:
				m_imd_obj = GPOS_NEW(m_memory_pool) CMDTypeInt2GPDB(m_memory_pool);
				break;

			case GPDB_INT4:
				m_imd_obj = GPOS_NEW(m_memory_pool) CMDTypeInt4GPDB(m_memory_pool);
				break;

			case GPDB_INT8:
				m_imd_obj = GPOS_NEW(m_memory_pool) CMDTypeInt8GPDB(m_memory_pool);
				break;

			case GPDB_BOOL:
				m_imd_obj = GPOS_NEW(m_memory_pool) CMDTypeBoolGPDB(m_memory_pool);
				break;

			case GPDB_OID:
				m_imd_obj = GPOS_NEW(m_memory_pool) CMDTypeOidGPDB(m_memory_pool);
				break;

			default:
				m_mdid->AddRef();
				m_pmdidOpEq->AddRef();
				m_pmdidOpNEq->AddRef();
				m_pmdidOpLT->AddRef();
				m_pmdidOpLEq->AddRef();
				m_pmdidOpGT->AddRef();
				m_pmdidOpGEq->AddRef();
				m_pmdidOpComp->AddRef();
				m_pmdidMin->AddRef();
				m_pmdidMax->AddRef();
				m_pmdidAvg->AddRef();
				m_pmdidSum->AddRef();
				m_pmdidCount->AddRef();
				if(NULL != m_pmdidBaseRelation)
				{
					m_pmdidBaseRelation->AddRef();
				}
				m_pmdidTypeArray->AddRef();
				
				ULONG ulLen = 0;
				if (0 < m_iLength)
				{
					ulLen = (ULONG) m_iLength;
				}
				m_imd_obj = GPOS_NEW(m_memory_pool) CMDTypeGenericGPDB
										(
										m_memory_pool,
										m_mdid,
										m_mdname,
										m_fRedistributable,
										m_fFixedLength,
										ulLen,
										m_fByValue,
										m_pmdidOpEq,
										m_pmdidOpNEq,
										m_pmdidOpLT,
										m_pmdidOpLEq,
										m_pmdidOpGT,
										m_pmdidOpGEq,
										m_pmdidOpComp,
										m_pmdidMin,
										m_pmdidMax,
										m_pmdidAvg,
										m_pmdidSum,
										m_pmdidCount,
										m_fHashable,
										m_fComposite,
										m_pmdidBaseRelation,
										m_pmdidTypeArray,
										m_iLength
										);
				break;
		}
		
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();
	}
}

// EOF
