//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMetadataColumn.cpp
//
//	@doc:
//		Implementation of the SAX parse handler class for parsing column metadata.
//---------------------------------------------------------------------------

#include "naucrates/dxl/parser/CParseHandlerMetadataColumn.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataColumns.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataIdList.h"
#include "naucrates/dxl/parser/CParseHandlerScalarOp.h"

#include "naucrates/dxl/operators/CDXLOperatorFactory.h"

using namespace gpdxl;


XERCES_CPP_NAMESPACE_USE

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataColumn::CParseHandlerMetadataColumn
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CParseHandlerMetadataColumn::CParseHandlerMetadataColumn
	(
	IMemoryPool *memory_pool,
	CParseHandlerManager *parse_handler_mgr,
	CParseHandlerBase *parse_handler_root
	)
	:
	CParseHandlerBase(memory_pool, parse_handler_mgr, parse_handler_root),
	m_pmdcol(NULL),
	m_mdname(NULL),
	m_mdid_type(NULL),
	m_pdxlnDefaultValue(NULL),
	m_ulWidth(ULONG_MAX)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataColumn::~CParseHandlerMetadataColumn
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CParseHandlerMetadataColumn::~CParseHandlerMetadataColumn()
{
	CRefCount::SafeRelease(m_pmdcol);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataColumn::StartElement
//
//	@doc:
//		Invoked by Xerces to process an opening tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMetadataColumn::StartElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const, // element_qname
	const Attributes& attrs
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumn), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
	
	// parse column name
	const XMLCh *xmlszColumnName = CDXLOperatorFactory::ExtractAttrValue
												(
												attrs,
												EdxltokenName,
												EdxltokenMetadataColumn
												);

	CWStringDynamic *pstrColumnName = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), xmlszColumnName);
	
	// create a copy of the string in the CMDName constructor
	m_mdname = GPOS_NEW(m_memory_pool) CMDName(m_memory_pool, pstrColumnName);
	
	GPOS_DELETE(pstrColumnName);
	
	// parse attribute number
	m_iAttNo = CDXLOperatorFactory::ExtractConvertAttrValueToInt
								(
								m_parse_handler_mgr->GetDXLMemoryManager(),
								attrs,
								EdxltokenAttno,
								EdxltokenMetadataColumn
								);

	m_mdid_type = CDXLOperatorFactory::PmdidFromAttrs
								(
								m_parse_handler_mgr->GetDXLMemoryManager(),
								attrs,
								EdxltokenMdid,
								EdxltokenMetadataColumn
								);

	// parse optional type modifier
	m_type_modifier = CDXLOperatorFactory::ExtractConvertAttrValueToInt
								(
								m_parse_handler_mgr->GetDXLMemoryManager(),
								attrs,
								EdxltokenTypeMod,
								EdxltokenColDescr,
								true,
								IDefaultTypeModifier
								);

	// parse attribute number
	m_fNullable = CDXLOperatorFactory::ExtractConvertAttrValueToBool
								(
								m_parse_handler_mgr->GetDXLMemoryManager(),
								attrs,
								EdxltokenColumnNullable,
								EdxltokenMetadataColumn
								);

	// parse column length from attributes
	const XMLCh *xmlszColumnLength =  attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColWidth));

	if (NULL != xmlszColumnLength)
	{
		m_ulWidth = CDXLOperatorFactory::ConvertAttrValueToUlong
						(
						m_parse_handler_mgr->GetDXLMemoryManager(),
						xmlszColumnLength,
						EdxltokenColWidth,
						EdxltokenColDescr
						);
	}

	m_fDropped = false;
	const XMLCh *xmlszDropped = attrs.getValue(CDXLTokens::XmlstrToken(EdxltokenColDropped));

	if (NULL != xmlszDropped)
	{
		m_fDropped = CDXLOperatorFactory::ConvertAttrValueToBool
						(
						m_parse_handler_mgr->GetDXLMemoryManager(),
						xmlszDropped,
						EdxltokenColDropped,
						EdxltokenMetadataColumn
						);
	}
	
	// install a parse handler for the default value
	CParseHandlerBase *pph = CParseHandlerFactory::GetParseHandler
										(
										m_memory_pool,
										CDXLTokens::XmlstrToken(EdxltokenColumnDefaultValue),
										m_parse_handler_mgr,
										this
										);
		
	// activate and store parse handler
	m_parse_handler_mgr->ActivateParseHandler(pph);
	this->Append(pph);
}

//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataColumn::EndElement
//
//	@doc:
//		Invoked by Xerces to process a closing tag
//
//---------------------------------------------------------------------------
void
CParseHandlerMetadataColumn::EndElement
	(
	const XMLCh* const, // element_uri,
	const XMLCh* const element_local_name,
	const XMLCh* const // element_qname
	)
{
	if(0 != XMLString::compareString(CDXLTokens::XmlstrToken(EdxltokenColumn), element_local_name))
	{
		CWStringDynamic *str = CDXLUtils::CreateDynamicStringFromXMLChArray(m_parse_handler_mgr->GetDXLMemoryManager(), element_local_name);
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLUnexpectedTag, str->GetBuffer());
	}
	
	GPOS_ASSERT(1 == this->Length());
	
	// get node for default value expression from child parse handler
	CParseHandlerScalarOp *op_parse_handler = dynamic_cast<CParseHandlerScalarOp *>((*this)[0]);
	
	m_pdxlnDefaultValue = op_parse_handler->CreateDXLNode();
	
	if (NULL != m_pdxlnDefaultValue)
	{
		m_pdxlnDefaultValue->AddRef();
	}
	
	m_pmdcol = GPOS_NEW(m_memory_pool) CMDColumn
							(
							m_mdname,
							m_iAttNo,
							m_mdid_type,
							m_type_modifier,
							m_fNullable,
							m_fDropped,
							m_pdxlnDefaultValue,
							m_ulWidth
							);

	// deactivate handler
	m_parse_handler_mgr->DeactivateHandler();
}


//---------------------------------------------------------------------------
//	@function:
//		CParseHandlerMetadataColumn::Pmdcol
//
//	@doc:
//		Return the constructed list of metadata columns
//
//---------------------------------------------------------------------------
CMDColumn *
CParseHandlerMetadataColumn::Pmdcol()
{
	return m_pmdcol;
}

// EOF
