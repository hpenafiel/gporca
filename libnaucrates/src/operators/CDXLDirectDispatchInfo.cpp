//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLDirectDispatchInfo.cpp
//
//	@doc:
//		Implementation of DXL datum of types having LINT mapping
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLDirectDispatchInfo::CDXLDirectDispatchInfo
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo::CDXLDirectDispatchInfo
	(
	DXLDatumArrays *pdrgpdrgdxldatum
	)
	:
	m_pdrgpdrgpdxldatum(pdrgpdrgdxldatum)
{
	GPOS_ASSERT(NULL != pdrgpdrgdxldatum);
	
#ifdef GPOS_DEBUG
	const ULONG length = pdrgpdrgdxldatum->Size();
	if (0 < length)
	{
		ULONG ulDatums = ((*pdrgpdrgdxldatum)[0])->Size();
		for (ULONG ul = 1; ul < length; ul++)
		{
			GPOS_ASSERT(ulDatums == ((*pdrgpdrgdxldatum)[ul])->Size());
		}
	}
#endif // GPOS_DEBUG
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDirectDispatchInfo::~CDXLDirectDispatchInfo
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLDirectDispatchInfo::~CDXLDirectDispatchInfo()
{
	m_pdrgpdrgpdxldatum->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLDirectDispatchInfo::Serialize
//
//	@doc:
//		Serialize direct dispatch info in DXL format
//
//---------------------------------------------------------------------------
void
CDXLDirectDispatchInfo::Serialize
	(
	CXMLSerializer *xml_serializer
	)
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDirectDispatchInfo));
	
	const ULONG ulValueCombinations = (m_pdrgpdrgpdxldatum == NULL) ? 0 : m_pdrgpdrgpdxldatum->Size();
	
	for (ULONG ulA = 0; ulA < ulValueCombinations; ulA++)
	{
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDirectDispatchKeyValue));

		DXLDatumArray *pdrgpdxldatum = (*m_pdrgpdrgpdxldatum)[ulA];
		
		const ULONG ulDatums = pdrgpdxldatum->Size();
		for (ULONG ulB = 0; ulB < ulDatums; ulB++) 
		{
			CDXLDatum *pdxldatum = (*pdrgpdxldatum)[ulB];
			pdxldatum->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenDatum));
		}
		
		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDirectDispatchKeyValue));
	}
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDirectDispatchInfo));
}

// EOF
