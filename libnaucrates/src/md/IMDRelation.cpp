//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		IMDRelation.cpp
//
//	@doc:
//		Implementation
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDRelation.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		IMDRelation::PstrDistrPolicy
//
//	@doc:
//		Return relation distribution policy as a string value
//
//---------------------------------------------------------------------------
const CWStringConst *
IMDRelation::PstrDistrPolicy
	(
	GetRelDistrPolicy rel_distr_policy
	)
{
	switch (rel_distr_policy)
	{
		case EreldistrMasterOnly:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelDistrMasterOnly);
		case EreldistrHash:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelDistrHash);
		case EreldistrRandom:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelDistrRandom);
		default:
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		IMDRelation::PstrStorageType
//
//	@doc:
//		Return name of storage type
//
//---------------------------------------------------------------------------
const CWStringConst *
IMDRelation::PstrStorageType
	(
	IMDRelation::Erelstoragetype rel_storage_type
	)
{
	switch (rel_storage_type)
	{
		case ErelstorageHeap:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageHeap);
		case ErelstorageAppendOnlyCols:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageAppendOnlyCols);
		case ErelstorageAppendOnlyRows:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageAppendOnlyRows);
		case ErelstorageAppendOnlyParquet:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageAppendOnlyParquet);
		case ErelstorageExternal:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageExternal);
		case ErelstorageVirtual:
			return CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageVirtual);
		default:
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		IMDRelation::PstrColumns
//
//	@doc:
//		Serialize an array of column ids into a comma-separated string
//
//---------------------------------------------------------------------------
CWStringDynamic *
IMDRelation::PstrColumns
	(
	IMemoryPool *memory_pool,
	ULongPtrArray *pdrgpul
	)
{
	CWStringDynamic *str = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	ULONG ulLen = pdrgpul->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		ULONG id = *((*pdrgpul)[ul]);
		if (ul == ulLen - 1)
		{
			// last element: do not print a comma
			str->AppendFormat(GPOS_WSZ_LIT("%d"), id);
		}
		else
		{
			str->AppendFormat(GPOS_WSZ_LIT("%d%ls"), id, CDXLTokens::GetDXLTokenStr(EdxltokenComma)->GetBuffer());
		}
	}

	return str;
}

// check if index is partial given its mdid
BOOL
IMDRelation::FPartialIndex
	(
	IMDId * // mdid
	) const
{
	return false;
}

// EOF
