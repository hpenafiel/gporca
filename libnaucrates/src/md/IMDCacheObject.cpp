//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		IMDCacheObject.cpp
//
//	@doc:
//		Implementation of common methods for MD cache objects
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDCacheObject.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		IMDCacheObject::SerializeMDIdAsElem
//
//	@doc:
//		Serialize MD operator info in DXL format
//
//---------------------------------------------------------------------------
void
IMDCacheObject::SerializeMDIdAsElem
	(
	CXMLSerializer *xml_serializer,
	const CWStringConst *pstrElem,
	const IMDId *pmdid
	)
	const
{
	if (NULL == pmdid)
	{
		return;
	}
	
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						pstrElem);
	
	pmdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
							pstrElem);
}


//---------------------------------------------------------------------------
//	@function:
//		IMDCacheObject::SerializeMDIdList
//
//	@doc:
//		Serialize a list of metadata ids into DXL
//
//---------------------------------------------------------------------------
void
IMDCacheObject::SerializeMDIdList
	(
	CXMLSerializer *xml_serializer,
	const DrgPmdid *mdid_array,
	const CWStringConst *pstrTokenList,
	const CWStringConst *pstrTokenListItem
	)
{
	// serialize list of metadata ids
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrTokenList);
	const ULONG ulLen = mdid_array->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrTokenListItem);

		IMDId *pmdid = (*mdid_array)[ul];
		pmdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
		xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrTokenListItem);

		GPOS_CHECK_ABORT;
	}

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrTokenList);
}

// EOF

