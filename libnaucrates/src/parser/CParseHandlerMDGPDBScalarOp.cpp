//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CParseHandlerMDGPDBScalarOp.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing metadata for
//		GPDB scalar operators.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMDGPDBScalarOp.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

#include "naucrates/md/CMDScalarOpGPDB.h"

using namespace gpdxl;
using namespace gpmd;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBScalarOp::CParseHandlerMDGPDBScalarOp
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMDGPDBScalarOp::CParseHandlerMDGPDBScalarOp
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerMetadataObject(memory_pool, parse_handler_mgr, parse_handler_root),
	m_mdid(NULL),
	m_mdname(NULL),
	m_pmdidTypeLeft(NULL),
	m_pmdidTypeRight(NULL),
	m_pmdidTypeResult(NULL),
	m_func_mdid(NULL),
	m_pmdidOpCommute(NULL),
	m_pmdidOpInverse(NULL),
	m_ecmpt(IMDType::EcmptOther),
	m_fReturnsNullOnNullInput(false)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBScalarOp::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBScalarOp::StartElement
	(
	const XMLCh* const element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const element_qname,
	const Attributes& attrs
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOp), element_local_name))
	{
		// parse operator name
		const XMLCh *xmlszOpName = CDXLOperatorFactory::ExtractAttrValue
															(
															attrs,
															EdxltokenName,
															EdxltokenGPDBScalarOp
															);

		CWStringDynamic *pstrOpName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszOpName);
		
		// create a copy of the string in the CMDName constructor
		m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrOpName);
		
		GPOS_DELETE(pstrOpName);

		// parse metadata id info
		m_mdid = CDXLOperatorFactory::PmdidFromAttrs
										(
										m_parse_handler_mgr->GetDXLMemoryManager(),
										attrs,
										EdxltokenMdid,
										EdxltokenGPDBScalarOp
										);
		
		const XMLCh *xmlszCmpType = CDXLOperatorFactory::ExtractAttrValue
									(
									attrs,
									EdxltokenGPDBScalarOpCmpType,
									EdxltokenGPDBScalarOp
									);

		m_ecmpt = CDXLOperatorFactory::Ecmpt(xmlszCmpType);

		// null-returning property is optional
		const XMLCh *xmlszReturnsNullOnNullInput = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenReturnsNullOnNullInput));
		if (NULL != xmlszReturnsNullOnNullInput)
		{
			m_fReturnsNullOnNullInput = CDXLOperatorFactory::ExtractConvertAttrValueToBool
								(
								m_parse_handler_mgr->GetDXLMemoryManager(),
								attrs,
								EdxltokenReturnsNullOnNullInput,
								EdxltokenGPDBScalarOp
								);
		}

	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpLeftTypeId), element_local_name))
	{
		// parse left operand's type
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidTypeLeft = CDXLOperatorFactory::PmdidFromAttrs
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												attrs, 
												EdxltokenMdid,
												EdxltokenGPDBScalarOpLeftTypeId
												);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpRightTypeId), element_local_name))
	{
		// parse right operand's type
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidTypeRight = CDXLOperatorFactory::PmdidFromAttrs
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													attrs,
													EdxltokenMdid,
													EdxltokenGPDBScalarOpRightTypeId
													);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpResultTypeId), element_local_name))
	{
		// parse result type
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidTypeResult = CDXLOperatorFactory::PmdidFromAttrs
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													attrs,
													EdxltokenMdid,
													EdxltokenGPDBScalarOpResultTypeId
													);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpFuncId), element_local_name))
	{
		// parse op func id
		GPOS_ASSERT(NULL != m_mdname);

		m_func_mdid = CDXLOperatorFactory::PmdidFromAttrs
												(
												m_parse_handler_mgr->GetDXLMemoryManager(),
												attrs, 
												EdxltokenMdid,
												EdxltokenGPDBScalarOpFuncId
												);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpCommOpId), element_local_name))
	{
		// parse commutator operator
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidOpCommute = CDXLOperatorFactory::PmdidFromAttrs
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													attrs,
													EdxltokenMdid,
													EdxltokenGPDBScalarOpCommOpId
													);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpInverseOpId), element_local_name))
	{
		// parse inverse operator id
		GPOS_ASSERT(NULL != m_mdname);

		m_pmdidOpInverse = CDXLOperatorFactory::PmdidFromAttrs
													(
													m_parse_handler_mgr->GetDXLMemoryManager(),
													attrs,
													EdxltokenMdid,
													EdxltokenGPDBScalarOpInverseOpId
													);
	}
	else if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenOpClasses), element_local_name))
	{
		// parse handler for operator class list
		CParseHandlerBase *pphOpClassList = CParseHandlerFactory::GetParseHandler(m_memory_pool, CDXLTokens::XmlstrToken(EdxltokenMetadataIdList), m_parse_handler_mgr, this);
		m_parse_handler_mgr->ActivateParseHandler(pphOpClassList);
		this->Append(pphOpClassList);
		pphOpClassList->startElement(element_uri, element_local_name, element_qname, attrs);
	}
	else
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBScalarOp::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMDGPDBScalarOp::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOp), element_local_name))
	{
		// construct the MD scalar operator object from its part
		GPOS_ASSERT(m_mdid->IsValid() && NULL != m_mdname);
		
		GPOS_ASSERT(0 == this->Length() || 1 == this->Length());
		
		DrgPmdid *pdrgpmdidOpClasses = NULL;
		if (0 < this->Length())
		{
			CParseHandlerMetadataIdList *pphMdidOpClasses = dynamic_cast<CParseHandlerMetadataIdList*>((*this)[0]);
			pdrgpmdidOpClasses = pphMdidOpClasses->GetMdIdArray();
			pdrgpmdidOpClasses->AddRef();
		}
		else 
		{
			pdrgpmdidOpClasses = GPOS_NEW(m_memory_pool) DrgPmdid(m_memory_pool);
		}
		m_imd_obj = GPOS_NEW(m_memory_pool) CMDScalarOpGPDB
				(
				m_memory_pool,
				m_mdid,
				m_mdname,
				m_pmdidTypeLeft,
				m_pmdidTypeRight,
				m_pmdidTypeResult,
				m_func_mdid,
				m_pmdidOpCommute,
				m_pmdidOpInverse,
				m_ecmpt,
				m_fReturnsNullOnNullInput,
				pdrgpmdidOpClasses
				)
				;
		
		// deactivate handler
		m_parse_handler_mgr->DeactivateHandler();

	}
	else if (!FSupportedChildElem(element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMDGPDBScalarOp::FSupportedElem
//
//	@doc:
//		Is this a supported child elem of the scalar op
//
//---------------------------------------------------------------------------
BOOL
CParseHandlerMDGPDBScalarOp::FSupportedChildElem
	(
	const XMLCh* const xmlsz
	)
{
	return (0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpLeftTypeId), xmlsz) ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpRightTypeId), xmlsz) ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpResultTypeId), xmlsz) ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpFuncId), xmlsz) ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpCommOpId), xmlsz) ||
			0 == XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenGPDBScalarOpInverseOpId), xmlsz));
}

// EOF
