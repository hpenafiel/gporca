//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDRelationGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing metadata cache relations
//---------------------------------------------------------------------------


#include "gpos/string/CWStringDynamic.h"

#include "naucrates/exception.h"
#include "naucrates/md/CMDRelationGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::CMDRelationGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDRelationGPDB::CMDRelationGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	BOOL fTemporary,
	Erelstoragetype rel_storage_type,
	Ereldistrpolicy rel_distr_policy,
	DrgPmdcol *pdrgpmdcol,
	ULongPtrArray *pdrgpulDistrColumns,
	ULongPtrArray *pdrgpulPartColumns,
	CharPtrArray *pdrgpszPartTypes,
	ULONG ulPartitions,
	BOOL fConvertHashToRandom,
	ULongPtrArray2D *pdrgpdrgpulKeys,
	DrgPmdIndexInfo *pdrgpmdIndexInfo,
	DrgPmdid *pdrgpmdidTriggers,
 	DrgPmdid *pdrgpmdidCheckConstraint,
 	IMDPartConstraint *pmdpartcnstr,
 	BOOL fHasOids
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname(mdname),
	m_is_temp_table(fTemporary),
	m_rel_storage_type(rel_storage_type),
	m_rel_distr_policy(rel_distr_policy),
	m_pdrgpmdcol(pdrgpmdcol),
	m_ulDroppedCols(0),
	m_pdrgpulDistrColumns(pdrgpulDistrColumns),
	m_fConvertHashToRandom(fConvertHashToRandom),
	m_pdrgpulPartColumns(pdrgpulPartColumns),
	m_pdrgpszPartTypes(pdrgpszPartTypes),
	m_ulPartitions(ulPartitions),
	m_pdrgpdrgpulKeys(pdrgpdrgpulKeys),
	m_pdrgpmdIndexInfo(pdrgpmdIndexInfo),
	m_pdrgpmdidTriggers(pdrgpmdidTriggers),
	m_pdrgpmdidCheckConstraint(pdrgpmdidCheckConstraint),
	m_pmdpartcnstr(pmdpartcnstr),
	m_has_oids(fHasOids),
	m_ulSystemColumns(0),
	m_phmululNonDroppedCols(NULL),
	m_phmiulAttno2Pos(NULL),
	m_pdrgpulNonDroppedCols(NULL)
{
	GPOS_ASSERT(pmdid->IsValid());
	GPOS_ASSERT(NULL != pdrgpmdcol);
	GPOS_ASSERT(NULL != pdrgpmdIndexInfo);
	GPOS_ASSERT(NULL != pdrgpmdidTriggers);
	GPOS_ASSERT(NULL != pdrgpmdidCheckConstraint);
	GPOS_ASSERT_IMP(fConvertHashToRandom,
			IMDRelation::EreldistrHash == rel_distr_policy &&
			"Converting hash distributed table to random only possible for hash distributed tables");
	
	m_phmululNonDroppedCols = GPOS_NEW(m_memory_pool) HMUlUl(m_memory_pool);
	m_phmiulAttno2Pos = GPOS_NEW(m_memory_pool) HMIUl(m_memory_pool);
	m_pdrgpulNonDroppedCols = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	m_pdrgpdoubleColWidths = GPOS_NEW(memory_pool) DrgPdouble(memory_pool);

	const ULONG ulArity = pdrgpmdcol->Size();
	ULONG ulPosNonDropped = 0;
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		IMDColumn *pmdcol = (*pdrgpmdcol)[ul];
		BOOL fSystemCol = pmdcol->FSystemColumn();
		if (fSystemCol)
		{
			m_ulSystemColumns++;
		}

		(void) m_phmiulAttno2Pos->Insert
									(
									GPOS_NEW(m_memory_pool) INT(pmdcol->AttrNum()),
									GPOS_NEW(m_memory_pool) ULONG(ul)
									);

		if (pmdcol->IsDropped())
		{
			m_ulDroppedCols++;
		}
		else	
		{
			if (!fSystemCol)
			{
				m_pdrgpulNonDroppedCols->Append(GPOS_NEW(m_memory_pool) ULONG(ul));
			}
			(void) m_phmululNonDroppedCols->Insert(GPOS_NEW(m_memory_pool) ULONG(ul), GPOS_NEW(m_memory_pool) ULONG(ulPosNonDropped));
			ulPosNonDropped++;
		}

		m_pdrgpdoubleColWidths->Append(GPOS_NEW(memory_pool) CDouble(pmdcol->Length()));
	}
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::~CMDRelationGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDRelationGPDB::~CMDRelationGPDB()
{
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_mdid->Release();
	m_pdrgpmdcol->Release();
	CRefCount::SafeRelease(m_pdrgpulDistrColumns);
	CRefCount::SafeRelease(m_pdrgpulPartColumns);
	CRefCount::SafeRelease(m_pdrgpszPartTypes);
	CRefCount::SafeRelease(m_pdrgpdrgpulKeys);
	m_pdrgpmdIndexInfo->Release();
	m_pdrgpmdidTriggers->Release();
	m_pdrgpmdidCheckConstraint->Release();
	m_pdrgpdoubleColWidths->Release();
	CRefCount::SafeRelease(m_pmdpartcnstr);
	CRefCount::SafeRelease(m_phmululNonDroppedCols);
	CRefCount::SafeRelease(m_phmiulAttno2Pos);
	CRefCount::SafeRelease(m_pdrgpulNonDroppedCols);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::MDId
//
//	@doc:
//		Returns the metadata id of this relation
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::Mdname
//
//	@doc:
//		Returns the name of this relation
//
//---------------------------------------------------------------------------
CMDName
CMDRelationGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::FTemporary
//
//	@doc:
//		Is the relation temporary
//
//---------------------------------------------------------------------------
BOOL
CMDRelationGPDB::FTemporary() const
{
	return m_is_temp_table;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::Erelstorage
//
//	@doc:
//		Returns the storage type for this relation
//
//---------------------------------------------------------------------------
IMDRelation::Erelstoragetype
CMDRelationGPDB::Erelstorage() const
{
	return m_rel_storage_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::Ereldistribution
//
//	@doc:
//		Returns the distribution policy for this relation
//
//---------------------------------------------------------------------------
IMDRelation::Ereldistrpolicy
CMDRelationGPDB::Ereldistribution() const
{
	return m_rel_distr_policy;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlColumns
//
//	@doc:
//		Returns the number of columns of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlColumns() const
{
	GPOS_ASSERT(NULL != m_pdrgpmdcol);
	
	return m_pdrgpmdcol->Size();
}

// Return the width of a column with regards to the position
DOUBLE
CMDRelationGPDB::DColWidth
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgpdoubleColWidths)[ulPos]->Get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::FHasDroppedColumns
//
//	@doc:
//		Does relation have dropped columns
//
//---------------------------------------------------------------------------
BOOL
CMDRelationGPDB::FHasDroppedColumns() const
{	
	return 0 < m_ulDroppedCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlNonDroppedCols
//
//	@doc:
//		Number of non-dropped columns
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlNonDroppedCols() const
{	
	return UlColumns() - m_ulDroppedCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlPosNonDropped
//
//	@doc:
//		Return the absolute position of the given attribute position excluding 
//		dropped columns
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlPosNonDropped
	(
	ULONG ulPos
	)
	const
{	
	GPOS_ASSERT(ulPos <= UlColumns());
	
	if (!FHasDroppedColumns())
	{
		return ulPos;
	}
	
	ULONG *pul = m_phmululNonDroppedCols->Find(&ulPos);
	
	GPOS_ASSERT(NULL != pul);
	return *pul;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlPosFromAttno
//
//	@doc:
//		Return the position of a column in the metadata object given the
//      attribute number in the system catalog
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlPosFromAttno
	(
	INT iAttno
	)
	const
{
	ULONG *pul = m_phmiulAttno2Pos->Find(&iAttno);
	GPOS_ASSERT(NULL != pul);

	return *pul;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PdrgpulNonDroppedCols
//
//	@doc:
//		Returns the original positions of all the non-dropped columns
//
//---------------------------------------------------------------------------
ULongPtrArray *
CMDRelationGPDB::PdrgpulNonDroppedCols() const
{
	return m_pdrgpulNonDroppedCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlSystemColumns
//
//	@doc:
//		Returns the number of system columns of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlSystemColumns() const
{
	return m_ulSystemColumns;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlKeySets
//
//	@doc:
//		Returns the number of key sets
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlKeySets() const
{	
	return (m_pdrgpdrgpulKeys == NULL) ? 0 : m_pdrgpdrgpulKeys->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PdrgpulKeyset
//
//	@doc:
//		Returns the key set at the specified position
//
//---------------------------------------------------------------------------
const ULongPtrArray *
CMDRelationGPDB::PdrgpulKeyset
	(
	ULONG ulPos
	) 
	const
{	
	GPOS_ASSERT(NULL != m_pdrgpdrgpulKeys);
	
	return (*m_pdrgpdrgpulKeys)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlDistrColumns
//
//	@doc:
//		Returns the number of columns in the distribution column list of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlDistrColumns() const
{	
	return (m_pdrgpulDistrColumns == NULL) ? 0 : m_pdrgpulDistrColumns->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::FHasOids
//
//	@doc:
//		Does this table have oids
//
//---------------------------------------------------------------------------
BOOL
CMDRelationGPDB::FHasOids() const
{
	return m_has_oids;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::FPartitioned
//
//	@doc:
//		Is the table partitioned
//
//---------------------------------------------------------------------------
BOOL
CMDRelationGPDB::FPartitioned() const
{	
	return (0 < UlPartColumns());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlPartitions
//
//	@doc:
//		number of partitions
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlPartitions() const
{
	return m_ulPartitions;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlPartColumns
//
//	@doc:
//		Returns the number of partition keys
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlPartColumns() const
{	
	return (m_pdrgpulPartColumns == NULL) ? 0 : m_pdrgpulPartColumns->Size();
}

// Retrieve list of partition types
CharPtrArray *
CMDRelationGPDB::PdrgpszPartTypes() const
{
	return m_pdrgpszPartTypes;
}

// Returns the partition type of the given level
CHAR
CMDRelationGPDB::SzPartType(ULONG ulLevel) const
{
	return *(*m_pdrgpszPartTypes)[ulLevel];
}


//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PmdcolPartColumn
//
//	@doc:
//		Returns the partition column at the specified position in the
//		partition key list
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationGPDB::PmdcolPartColumn
	(
	ULONG ulPos
	) 
	const
{
	ULONG ulPartKeyPos = (*(*m_pdrgpulPartColumns)[ulPos]);
	return Pmdcol(ulPartKeyPos);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlIndices
//
//	@doc:
//		Returns the number of indices of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlIndices() const
{
	return m_pdrgpmdIndexInfo->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlTriggers
//
//	@doc:
//		Returns the number of triggers of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlTriggers() const
{
	return m_pdrgpmdidTriggers->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::Pmdcol
//
//	@doc:
//		Returns the column at the specified position
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationGPDB::Pmdcol
	(
	ULONG ulPos
	) 
	const
{
	GPOS_ASSERT(ulPos < m_pdrgpmdcol->Size());

	return (*m_pdrgpmdcol)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PmdcolDistrColumn
//
//	@doc:
//		Returns the distribution column at the specified position in the distribution column list
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationGPDB::PmdcolDistrColumn
	(
	ULONG ulPos
	) 
	const
{
	GPOS_ASSERT(ulPos < m_pdrgpulDistrColumns->Size());
	
	ULONG ulDistrKeyPos = (*(*m_pdrgpulDistrColumns)[ulPos]);
	return Pmdcol(ulDistrKeyPos);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::FConvertHashToRandom
//
//	@doc:
//		Return true if a hash distributed table needs to be considered as random during planning
//---------------------------------------------------------------------------
BOOL
CMDRelationGPDB::FConvertHashToRandom() const
{
	return m_fConvertHashToRandom;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PmdidIndex
//
//	@doc:
//		Returns the id of the index at the specified position of the index array
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationGPDB::PmdidIndex
	(
	ULONG ulPos
	) 
	const
{
	return (*m_pdrgpmdIndexInfo)[ulPos]->MDId();
}

// check if index is partial given its mdid
BOOL
CMDRelationGPDB::FPartialIndex
	(
	IMDId *pmdid
	)
	const
{
	const ULONG ulIndexes = UlIndices();

	for (ULONG ul = 0; ul < ulIndexes; ++ul)
	{
		if (CMDIdGPDB::FEqualMDId(PmdidIndex(ul), pmdid))
		{
			return (*m_pdrgpmdIndexInfo)[ul]->FPartial();
		}
	}

	// Not found
	GPOS_RAISE(ExmaMD, ExmiMDCacheEntryNotFound, pmdid->GetBuffer());

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PmdidTrigger
//
//	@doc:
//		Returns the id of the trigger at the specified position of the trigger array
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationGPDB::PmdidTrigger
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgpmdidTriggers)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::UlCheckConstraints
//
//	@doc:
//		Returns the number of check constraints on this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationGPDB::UlCheckConstraints() const
{
	return m_pdrgpmdidCheckConstraint->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::PmdidCheckConstraint
//
//	@doc:
//		Returns the id of the check constraint at the specified position of
//		the check constraint array
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationGPDB::PmdidCheckConstraint
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgpmdidCheckConstraint)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::Pmdpartcnstr
//
//	@doc:
//		Return the part constraint
//
//---------------------------------------------------------------------------
IMDPartConstraint *
CMDRelationGPDB::Pmdpartcnstr() const
{
	return m_pmdpartcnstr;
}


//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDRelationGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	) 
	const
{
	GPOS_CHECK_ABORT;

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenRelation));
	
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelTemporary), m_is_temp_table);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelHasOids), m_has_oids);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageType), IMDRelation::PstrStorageType(m_rel_storage_type));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelDistrPolicy), PstrDistrPolicy(m_rel_distr_policy));
	
	if (EreldistrHash == m_rel_distr_policy)
	{
		GPOS_ASSERT(NULL != m_pdrgpulDistrColumns);
		
		// serialize distribution columns
		CWStringDynamic *pstrDistrColumns = PstrColumns(m_memory_pool, m_pdrgpulDistrColumns);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDistrColumns), pstrDistrColumns);
		GPOS_DELETE(pstrDistrColumns);
	}
	
	// serialize key sets
	if (m_pdrgpdrgpulKeys != NULL && m_pdrgpdrgpulKeys->Size() > 0)
	{
		CWStringDynamic *pstrKeys = CDXLUtils::Serialize(m_memory_pool, m_pdrgpdrgpulKeys);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenKeys), pstrKeys);
		GPOS_DELETE(pstrKeys);
	}
	
	if (FPartitioned())
	{
		// serialize partition keys
		CWStringDynamic *pstrPartKeys = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulPartColumns);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPartKeys), pstrPartKeys);
		GPOS_DELETE(pstrPartKeys);
	}

	if (m_pdrgpszPartTypes)
	{
		// serialize partition types
		CWStringDynamic *pstrPartTypes = CDXLUtils::SerializeToCommaSeparatedString(m_memory_pool, m_pdrgpszPartTypes);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenPartTypes), pstrPartTypes);
		GPOS_DELETE(pstrPartTypes);
	}
	
	if (m_fConvertHashToRandom)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenConvertHashToRandom), m_fConvertHashToRandom);
	}

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenNumLeafPartitions), m_ulPartitions);

	// serialize columns
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenColumns));
	for (ULONG ul = 0; ul < m_pdrgpmdcol->Size(); ul++)
	{
		CMDColumn *pmdcol = (*m_pdrgpmdcol)[ul];
		pmdcol->Serialize(xml_serializer);

		GPOS_CHECK_ABORT;
	}
	
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenColumns));
	
	// serialize index infos
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenIndexInfoList));
	const ULONG ulIndexes = m_pdrgpmdIndexInfo->Size();
	for (ULONG ul = 0; ul < ulIndexes; ul++)
	{
		CMDIndexInfo *pmdIndexInfo = (*m_pdrgpmdIndexInfo)[ul];
		pmdIndexInfo->Serialize(xml_serializer);

		GPOS_CHECK_ABORT;
	}

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenIndexInfoList));


	// serialize trigger information
	SerializeMDIdList(xml_serializer, m_pdrgpmdidTriggers,
						CDXLTokens::GetDXLTokenStr(EdxltokenTriggers),
						CDXLTokens::GetDXLTokenStr(EdxltokenTrigger)); 

	// serialize check constraint information
	SerializeMDIdList(xml_serializer, m_pdrgpmdidCheckConstraint,
						CDXLTokens::GetDXLTokenStr(EdxltokenCheckConstraints),
						CDXLTokens::GetDXLTokenStr(EdxltokenCheckConstraint));

	// serialize part constraint
	if (NULL != m_pmdpartcnstr)
	{
		m_pmdpartcnstr->Serialize(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), 
						CDXLTokens::GetDXLTokenStr(EdxltokenRelation));

	GPOS_CHECK_ABORT;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDRelationGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDRelationGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "Relation id: ";
	MDId()->OsPrint(os);
	os << std::endl;
	
	os << "Relation name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;
	
	os << "Storage type: " << IMDRelation::PstrStorageType(m_rel_storage_type)->GetBuffer() << std::endl;
	
	os << "Distribution policy: " << PstrDistrPolicy(m_rel_distr_policy)->GetBuffer() << std::endl;
	
	os << "Relation columns: " << std::endl;
	const ULONG ulColumns = UlColumns(); 
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const IMDColumn *pimdcol = Pmdcol(ul);
		pimdcol->DebugPrint(os);
	}
	os << std::endl;
	
	os << "Distributed by: ";
	const ULONG ulDistrColumns = UlDistrColumns(); 
	for (ULONG ul = 0; ul < ulDistrColumns; ul++)
	{
		if (0 < ul)
		{
			os << ", ";
		}
		
		const IMDColumn *pimdcolDistrKey = PmdcolDistrColumn(ul);
		os << (pimdcolDistrKey->Mdname()).GetMDName()->GetBuffer();		
	}
	
	os << std::endl;

	os << "Partition keys: ";
	const ULONG ulPartColumns = UlPartColumns(); 
	for (ULONG ul = 0; ul < ulPartColumns; ul++)
	{
		if (0 < ul)
		{
			os << ", ";
		}
		
		const IMDColumn *pmdcolPartKey = PmdcolPartColumn(ul);
		os << (pmdcolPartKey->Mdname()).GetMDName()->GetBuffer();		
	}
		
	os << std::endl;
		
	os << "Index Info: ";
	const ULONG ulIndexes = m_pdrgpmdIndexInfo->Size();
	for (ULONG ul = 0; ul < ulIndexes; ul++)
	{
		CMDIndexInfo *pmdIndexInfo = (*m_pdrgpmdIndexInfo)[ul];
		pmdIndexInfo->DebugPrint(os);
	}

	os << "Triggers: ";
	CDXLUtils::DebugPrintMDIdArray(os, m_pdrgpmdidTriggers);

	os << "Check Constraint: ";
	CDXLUtils::DebugPrintMDIdArray(os, m_pdrgpmdidCheckConstraint);
}

#endif // GPOS_DEBUG

// EOF

