//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLCtasStorageOptions.cpp
//
//	@doc:
//		Implementation of DXL CTAS storage options
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::CDXLCtasStorageOptions
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions::CDXLCtasStorageOptions
	( 
	CMDName *pmdnameTablespace,
	ECtasOnCommitAction ectascommit,
	DrgPctasOpt *pdrgpctasopt
	)
	:
	m_pmdnameTablespace(pmdnameTablespace),
	m_ectascommit(ectascommit),
	m_pdrgpctasopt(pdrgpctasopt)
{
	GPOS_ASSERT(EctascommitSentinel > ectascommit);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::~CDXLCtasStorageOptions
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions::~CDXLCtasStorageOptions()
{
	GPOS_DELETE(m_pmdnameTablespace);
	CRefCount::SafeRelease(m_pdrgpctasopt);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::PmdnameTablespace
//
//	@doc:
//		Returns the tablespace name
//
//---------------------------------------------------------------------------
CMDName *
CDXLCtasStorageOptions::PmdnameTablespace() const
{
	return m_pmdnameTablespace;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::Ectascommit
//
//	@doc:
//		Returns the OnCommit ctas spec
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions::ECtasOnCommitAction
CDXLCtasStorageOptions::Ectascommit() const
{
	return m_ectascommit;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::DrgPctasOpt
//
//	@doc:
//		Returns array of storage options
//
//---------------------------------------------------------------------------
CDXLCtasStorageOptions::DrgPctasOpt *
CDXLCtasStorageOptions::Pdrgpctasopt() const
{
	return m_pdrgpctasopt;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::Serialize
//
//	@doc:
//		Serialize options in DXL format
//
//---------------------------------------------------------------------------
void
CDXLCtasStorageOptions::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenCTASOptions));
	if (NULL != m_pmdnameTablespace)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenTablespace), m_pmdnameTablespace->GetMDName());
	}
	
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitAction), PstrOnCommitAction(m_ectascommit));
	
	const ULONG ulOptions = (m_pdrgpctasopt == NULL) ? 0 : m_pdrgpctasopt->Size();
	for (ULONG ul = 0; ul < ulOptions; ul++)
	{
		CDXLCtasOption *pdxlctasopt = (*m_pdrgpctasopt)[ul];
		xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenCTASOption));
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenCtasOptionType), pdxlctasopt->m_ulType);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), pdxlctasopt->m_str_name);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenValue), pdxlctasopt->m_pstrValue);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIsNull), pdxlctasopt->m_is_null);
		xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenCTASOption));
	}
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenCTASOptions));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLCtasStorageOptions::PstrOnCommitAction
//
//	@doc:
//		String representation of OnCommit action spec
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLCtasStorageOptions::PstrOnCommitAction
	(
	CDXLCtasStorageOptions::ECtasOnCommitAction ectascommit
	) 
{
	switch (ectascommit)
	{
		case EctascommitNOOP:
			return CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitNOOP);
			
		case EctascommitPreserve:
			return CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitPreserve);
			
		case EctascommitDelete:
			return CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitDelete);
			
		case EctascommitDrop:
			return CDXLTokens::GetDXLTokenStr(EdxltokenOnCommitDrop);
		
		default:
			GPOS_ASSERT("Invalid on commit option");
			return NULL;
	}
}

// EOF
