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
	Ereldistrpolicy rel_distr_policy
	)
{
	switch (rel_distr_policy)
	{
		case EreldistrMasterOnly:
			return CDXLTokens::PstrToken(EdxltokenRelDistrMasterOnly);
		case EreldistrHash:
			return CDXLTokens::PstrToken(EdxltokenRelDistrHash);
		case EreldistrRandom:
			return CDXLTokens::PstrToken(EdxltokenRelDistrRandom);
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
			return CDXLTokens::PstrToken(EdxltokenRelStorageHeap);
		case ErelstorageAppendOnlyCols:
			return CDXLTokens::PstrToken(EdxltokenRelStorageAppendOnlyCols);
		case ErelstorageAppendOnlyRows:
			return CDXLTokens::PstrToken(EdxltokenRelStorageAppendOnlyRows);
		case ErelstorageAppendOnlyParquet:
			return CDXLTokens::PstrToken(EdxltokenRelStorageAppendOnlyParquet);
		case ErelstorageExternal:
			return CDXLTokens::PstrToken(EdxltokenRelStorageExternal);
		case ErelstorageVirtual:
			return CDXLTokens::PstrToken(EdxltokenRelStorageVirtual);
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
	CWStringDynamic *pstr = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	ULONG ulLen = pdrgpul->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		ULONG ulId = *((*pdrgpul)[ul]);
		if (ul == ulLen - 1)
		{
			// last element: do not print a comma
			pstr->AppendFormat(GPOS_WSZ_LIT("%d"), ulId);
		}
		else
		{
			pstr->AppendFormat(GPOS_WSZ_LIT("%d%ls"), ulId, CDXLTokens::PstrToken(EdxltokenComma)->GetBuffer());
		}
	}

	return pstr;
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
