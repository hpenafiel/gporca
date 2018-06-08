//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDIndexGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing metadata indexes
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDIndexGPDB.h"
#include "naucrates/md/CMDRelationGPDB.h"
#include "naucrates/md/IMDPartConstraint.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/exception.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::CMDIndexGPDB
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CMDIndexGPDB::CMDIndexGPDB
	(
	IMemoryPool *memory_pool, 
	IMDId *mdid, 
	CMDName *mdname,
	BOOL fClustered, 
	IMDIndex::EmdindexType emdindt,
	IMDId *pmdidItemType,
	ULongPtrArray *pdrgpulKeyCols,
	ULongPtrArray *pdrgpulIncludedCols,
	DrgPmdid *pdrgpmdidOpClasses,
	IMDPartConstraint *pmdpartcnstr
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(mdid),
	m_mdname(mdname),
	m_clustered(fClustered),
	m_index_type(emdindt),
	m_mdid_item_type(pmdidItemType),
	m_index_key_cols_array(pdrgpulKeyCols),
	m_included_cols_array(pdrgpulIncludedCols),
	m_pdrgpmdidOpClasses(pdrgpmdidOpClasses),
	m_pmdpartcnstr(pmdpartcnstr)
{
	GPOS_ASSERT(mdid->IsValid());
	GPOS_ASSERT(IMDIndex::EmdindSentinel > emdindt);
	GPOS_ASSERT(NULL != pdrgpulKeyCols);
	GPOS_ASSERT(0 < pdrgpulKeyCols->Size());
	GPOS_ASSERT(NULL != pdrgpulIncludedCols);
	GPOS_ASSERT_IMP(NULL != pmdidItemType, IMDIndex::EmdindBitmap == emdindt);
	GPOS_ASSERT_IMP(IMDIndex::EmdindBitmap == emdindt, NULL != pmdidItemType && pmdidItemType->IsValid());
	GPOS_ASSERT(NULL != pdrgpmdidOpClasses);
	
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::~CMDIndexGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIndexGPDB::~CMDIndexGPDB()
{
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_mdid->Release();
	CRefCount::SafeRelease(m_mdid_item_type);
	m_index_key_cols_array->Release();
	m_included_cols_array->Release();
	m_pdrgpmdidOpClasses->Release();
	CRefCount::SafeRelease(m_pmdpartcnstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::MDId
//
//	@doc:
//		Returns the metadata id of this index
//
//---------------------------------------------------------------------------
IMDId *
CMDIndexGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::Mdname
//
//	@doc:
//		Returns the name of this index
//
//---------------------------------------------------------------------------
CMDName
CMDIndexGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::FClustered
//
//	@doc:
//		Is the index clustered
//
//---------------------------------------------------------------------------
BOOL
CMDIndexGPDB::FClustered() const
{
	return m_clustered;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::Emdindt
//
//	@doc:
//		Index type
//
//---------------------------------------------------------------------------
IMDIndex::EmdindexType
CMDIndexGPDB::Emdindt() const
{
	return m_index_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::UlKeys
//
//	@doc:
//		Returns the number of index keys
//
//---------------------------------------------------------------------------
ULONG
CMDIndexGPDB::UlKeys() const
{
	return m_index_key_cols_array->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::UlKey
//
//	@doc:
//		Returns the n-th key column
//
//---------------------------------------------------------------------------
ULONG
CMDIndexGPDB::UlKey
	(
	ULONG ulPos
	) 
	const
{
	return *((*m_index_key_cols_array)[ulPos]);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::UlPosInKey
//
//	@doc:
//		Return the position of the key column in the index
//
//---------------------------------------------------------------------------
ULONG
CMDIndexGPDB::UlPosInKey
	(
	ULONG ulCol
	)
	const
{
	const ULONG size = UlKeys();

	for (ULONG ul = 0; ul < size; ul++)
	{
		if (UlKey(ul) == ulCol)
		{
			return ul;
		}
	}

	return ULONG_MAX;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::UlIncludedCols
//
//	@doc:
//		Returns the number of included columns
//
//---------------------------------------------------------------------------
ULONG
CMDIndexGPDB::UlIncludedCols() const
{
	return m_included_cols_array->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::UlIncludedCol
//
//	@doc:
//		Returns the n-th included column
//
//---------------------------------------------------------------------------
ULONG
CMDIndexGPDB::UlIncludedCol
	(
	ULONG ulPos
	)
	const
{
	return *((*m_included_cols_array)[ulPos]);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::UlPosInIncludedCol
//
//	@doc:
//		Return the position of the included column in the index
//
//---------------------------------------------------------------------------
ULONG
CMDIndexGPDB::UlPosInIncludedCol
	(
	ULONG ulCol
	)
	const
{
	const ULONG size = UlIncludedCols();

	for (ULONG ul = 0; ul < size; ul++)
	{
		if (UlIncludedCol(ul) == ulCol)
		{
			return ul;
		}
	}

	GPOS_ASSERT("Column not found in Index's included columns");

	return ULONG_MAX;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::Pmdpartcnstr
//
//	@doc:
//		Return the part constraint
//
//---------------------------------------------------------------------------
IMDPartConstraint *
CMDIndexGPDB::Pmdpartcnstr() const
{
	return m_pmdpartcnstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::Serialize
//
//	@doc:
//		Serialize MD index in DXL format
//
//---------------------------------------------------------------------------
void
CMDIndexGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenIndex));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIndexClustered), m_clustered);
	
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIndexType), PstrIndexType(m_index_type));
	if (NULL != m_mdid_item_type)
	{
		m_mdid_item_type->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenIndexItemType));
	}
		
	// serialize index keys
	CWStringDynamic *pstrKeyCols = CDXLUtils::Serialize(m_memory_pool, m_index_key_cols_array);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIndexKeyCols), pstrKeyCols);
	GPOS_DELETE(pstrKeyCols);

	CWStringDynamic *pstrAvailCols = CDXLUtils::Serialize(m_memory_pool, m_included_cols_array);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenIndexIncludedCols), pstrAvailCols);
	GPOS_DELETE(pstrAvailCols);
		
	// serialize operator class information
	SerializeMDIdList(xml_serializer, m_pdrgpmdidOpClasses, 
						CDXLTokens::GetDXLTokenStr(EdxltokenOpClasses), 
						CDXLTokens::GetDXLTokenStr(EdxltokenOpClass));
	
	if (NULL != m_pmdpartcnstr)
	{
		m_pmdpartcnstr->Serialize(xml_serializer);
	}
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenIndex));
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::DebugPrint
//
//	@doc:
//		Prints a MD index to the provided output
//
//---------------------------------------------------------------------------
void
CMDIndexGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Index id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Index name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	os << "Index type: " << PstrIndexType(m_index_type)->GetBuffer() << std::endl;

	os << "Index keys: ";
	for (ULONG ul = 0; ul < UlKeys(); ul++)
	{
		ULONG ulKey = UlKey(ul);
		if (ul > 0)
		{
			os << ", ";
		}
		os << ulKey;
	}
	os << std::endl;	

	os << "Included Columns: ";
	for (ULONG ul = 0; ul < UlIncludedCols(); ul++)
	{
		ULONG ulKey = UlIncludedCol(ul);
		if (ul > 0)
		{
			os << ", ";
		}
		os << ulKey;
	}
	os << std::endl;
}

#endif // GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::MDIdType
//
//	@doc:
//		Type of items returned by the index
//
//---------------------------------------------------------------------------
IMDId *
CMDIndexGPDB::PmdidItemType() const
{
	return m_mdid_item_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIndexGPDB::FCompatible
//
//	@doc:
//		Check if given scalar comparison can be used with the index key 
// 		at the specified position
//
//---------------------------------------------------------------------------
BOOL
CMDIndexGPDB::FCompatible
	(
	const IMDScalarOp *md_scalar_op, 
	ULONG ulKeyPos
	)
	const
{
	GPOS_ASSERT(NULL != md_scalar_op);
	GPOS_ASSERT(ulKeyPos < m_pdrgpmdidOpClasses->Size());
	
	// check if the index opclass for the key at the given position is one of 
	// the classes the scalar comparison belongs to
	const IMDId *pmdidOpClass = (*m_pdrgpmdidOpClasses)[ulKeyPos];
	
	const ULONG ulScOpClasses = md_scalar_op->UlOpCLasses();
	
	for (ULONG ul = 0; ul < ulScOpClasses; ul++)
	{
		if (pmdidOpClass->Equals(md_scalar_op->PmdidOpClass(ul)))
		{
			return true;
		}
	}
	
	return false;
}

// EOF

