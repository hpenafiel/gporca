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
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
						pstrElem);
	
	pmdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
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
	const DrgPmdid *pdrgpmdid,
	const CWStringConst *pstrTokenList,
	const CWStringConst *pstrTokenListItem
	)
{
	// serialize list of metadata ids
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenList);
	const ULONG ulLen = pdrgpmdid->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenListItem);

		IMDId *pmdid = (*pdrgpmdid)[ul];
		pmdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenListItem);

		GPOS_CHECK_ABORT;
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenList);
}

// EOF

