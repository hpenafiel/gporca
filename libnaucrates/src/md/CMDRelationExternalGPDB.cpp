//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDRelationExternalGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing MD cache external relations
//---------------------------------------------------------------------------

#include "naucrates/md/CMDRelationExternalGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::CMDRelationExternalGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDRelationExternalGPDB::CMDRelationExternalGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *mdname,
	GetRelDistrPolicy rel_distr_policy,
	DrgPmdcol *pdrgpmdcol,
	ULongPtrArray *pdrgpulDistrColumns,
	BOOL fConvertHashToRandom,
	ULongPtrArray2D *pdrgpdrgpulKeys,
	DrgPmdIndexInfo *pdrgpmdIndexInfo,
	DrgPmdid *pdrgpmdidTriggers,
 	DrgPmdid *pdrgpmdidCheckConstraint,
	INT iRejectLimit,
	BOOL fRejLimitInRows,
	IMDId *pmdidFmtErrRel
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname(mdname),
	m_rel_distr_policy(rel_distr_policy),
	m_pdrgpmdcol(pdrgpmdcol),
	m_ulDroppedCols(0),
	m_pdrgpulDistrColumns(pdrgpulDistrColumns),
	m_fConvertHashToRandom(fConvertHashToRandom),
	m_pdrgpdrgpulKeys(pdrgpdrgpulKeys),
	m_pdrgpmdIndexInfo(pdrgpmdIndexInfo),
	m_pdrgpmdidTriggers(pdrgpmdidTriggers),
	m_pdrgpmdidCheckConstraint(pdrgpmdidCheckConstraint),
	m_iRejectLimit(iRejectLimit),
	m_fRejLimitInRows(fRejLimitInRows),
	m_pmdidFmtErrRel(pmdidFmtErrRel),
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

	ULONG ulPosNonDropped = 0;
	const ULONG arity = pdrgpmdcol->Size();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		IMDColumn *pmdcol = (*pdrgpmdcol)[ul];

		BOOL fSystemCol = pmdcol->FSystemColumn();
		if (fSystemCol)
		{
			m_ulSystemColumns++;
		}
		
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

		(void) m_phmiulAttno2Pos->Insert
									(
									GPOS_NEW(m_memory_pool) INT(pmdcol->AttrNum()),
									GPOS_NEW(m_memory_pool) ULONG(ul)
									);
		m_pdrgpdoubleColWidths->Append(GPOS_NEW(memory_pool) CDouble(pmdcol->Length()));
	}
	m_pstr = CDXLUtils::SerializeMDObj(m_memory_pool, this, false /*fSerializeHeader*/, false /*indentation*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::~CMDRelationExternalGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDRelationExternalGPDB::~CMDRelationExternalGPDB()
{
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_mdid->Release();
	m_pdrgpmdcol->Release();
	CRefCount::SafeRelease(m_pdrgpulDistrColumns);
	CRefCount::SafeRelease(m_pdrgpdrgpulKeys);
	m_pdrgpmdIndexInfo->Release();
	m_pdrgpmdidTriggers->Release();
	m_pdrgpdoubleColWidths->Release();
	m_pdrgpmdidCheckConstraint->Release();
	CRefCount::SafeRelease(m_pmdidFmtErrRel);

	CRefCount::SafeRelease(m_phmululNonDroppedCols);
	CRefCount::SafeRelease(m_phmiulAttno2Pos);
	CRefCount::SafeRelease(m_pdrgpulNonDroppedCols);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::MDId
//
//	@doc:
//		Returns the metadata id of this relation
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationExternalGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::Mdname
//
//	@doc:
//		Returns the name of this relation
//
//---------------------------------------------------------------------------
CMDName
CMDRelationExternalGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::Ereldistribution
//
//	@doc:
//		Returns the distribution policy for this relation
//
//---------------------------------------------------------------------------
IMDRelation::GetRelDistrPolicy
CMDRelationExternalGPDB::Ereldistribution() const
{
	return m_rel_distr_policy;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlColumns
//
//	@doc:
//		Returns the number of columns of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlColumns() const
{
	GPOS_ASSERT(NULL != m_pdrgpmdcol);

	return m_pdrgpmdcol->Size();
}

// Return the width of a column with regards to the position
DOUBLE
CMDRelationExternalGPDB::DColWidth
(
	ULONG ulPos
	)
const
{
	return (*m_pdrgpdoubleColWidths)[ulPos]->Get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::FHasDroppedColumns
//
//	@doc:
//		Does relation have dropped columns
//
//---------------------------------------------------------------------------
BOOL
CMDRelationExternalGPDB::FHasDroppedColumns() const
{	
	return 0 < m_ulDroppedCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlNonDroppedCols
//
//	@doc:
//		Number of non-dropped columns
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlNonDroppedCols() const
{	
	return UlColumns() - m_ulDroppedCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlSystemColumns
//
//	@doc:
//		Returns the number of system columns of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlSystemColumns() const
{
	return m_ulSystemColumns;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::PdrgpulNonDroppedCols
//
//	@doc:
//		Returns the original positions of all the non-dropped columns
//
//---------------------------------------------------------------------------
ULongPtrArray *
CMDRelationExternalGPDB::PdrgpulNonDroppedCols() const
{
	return m_pdrgpulNonDroppedCols;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlPosNonDropped
//
//	@doc:
//		Return the absolute position of the given attribute position excluding 
//		dropped columns
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlPosNonDropped
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
//		CMDRelationExternalGPDB::UlPosFromAttno
//
//	@doc:
//		Return the position of a column in the metadata object given the
//		attribute number in the system catalog
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlPosFromAttno
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
//		CMDRelationExternalGPDB::FConvertHashToRandom
//
//	@doc:
//		Return true if a hash distributed table needs to be considered as random during planning
//---------------------------------------------------------------------------
BOOL
CMDRelationExternalGPDB::FConvertHashToRandom() const
{
	return m_fConvertHashToRandom;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::IRejectLimit
//
//	@doc:
//		Reject limit
//
//---------------------------------------------------------------------------
INT
CMDRelationExternalGPDB::IRejectLimit() const
{
	return m_iRejectLimit;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::FRejLimitInRows
//
//	@doc:
//		Is the reject limit in rows?
//
//---------------------------------------------------------------------------
BOOL
CMDRelationExternalGPDB::FRejLimitInRows() const
{
	return m_fRejLimitInRows;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::PmdidFmtErrRel
//
//	@doc:
//		Format error table mdid
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationExternalGPDB::PmdidFmtErrRel() const
{
	return m_pmdidFmtErrRel;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlKeySets
//
//	@doc:
//		Returns the number of key sets
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlKeySets() const
{
	return (m_pdrgpdrgpulKeys == NULL) ? 0 : m_pdrgpdrgpulKeys->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::PdrgpulKeyset
//
//	@doc:
//		Returns the key set at the specified position
//
//---------------------------------------------------------------------------
const ULongPtrArray *
CMDRelationExternalGPDB::PdrgpulKeyset
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
//		CMDRelationExternalGPDB::UlDistrColumns
//
//	@doc:
//		Returns the number of columns in the distribution column list of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlDistrColumns() const
{
	return (m_pdrgpulDistrColumns == NULL) ? 0 : m_pdrgpulDistrColumns->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlIndices
//
//	@doc:
//		Returns the number of indices of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlIndices() const
{
	return m_pdrgpmdIndexInfo->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlTriggers
//
//	@doc:
//		Returns the number of triggers of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlTriggers() const
{
	return m_pdrgpmdidTriggers->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::Pmdcol
//
//	@doc:
//		Returns the column at the specified position
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationExternalGPDB::Pmdcol
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
//		CMDRelationExternalGPDB::PmdcolDistrColumn
//
//	@doc:
//		Returns the distribution column at the specified position in the distribution column list
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationExternalGPDB::PmdcolDistrColumn
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
//		CMDRelationExternalGPDB::PmdidIndex
//
//	@doc:
//		Returns the id of the index at the specified position of the index array
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationExternalGPDB::PmdidIndex
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgpmdIndexInfo)[ulPos]->MDId();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::PmdidTrigger
//
//	@doc:
//		Returns the id of the trigger at the specified position of the trigger array
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationExternalGPDB::PmdidTrigger
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgpmdidTriggers)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::UlCheckConstraints
//
//	@doc:
//		Returns the number of check constraints on this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationExternalGPDB::UlCheckConstraints() const
{
	return m_pdrgpmdidCheckConstraint->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::PmdidCheckConstraint
//
//	@doc:
//		Returns the id of the check constraint at the specified position of
//		the check constraint array
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationExternalGPDB::PmdidCheckConstraint
	(
	ULONG ulPos
	)
	const
{
	return (*m_pdrgpmdidCheckConstraint)[ulPos];
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDRelationExternalGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenRelationExternal));

	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
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
	if (m_pdrgpdrgpulKeys != NULL && 0 < m_pdrgpdrgpulKeys->Size())
	{
		CWStringDynamic *pstrKeys = CDXLUtils::Serialize(m_memory_pool, m_pdrgpdrgpulKeys);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenKeys), pstrKeys);
		GPOS_DELETE(pstrKeys);
	}

	if (0 <= m_iRejectLimit)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenExtRelRejLimit), m_iRejectLimit);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenExtRelRejLimitInRows), m_fRejLimitInRows);
	}

	if (NULL != m_pmdidFmtErrRel)
	{
		m_pmdidFmtErrRel->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenExtRelFmtErrRel));
	}

	if (m_fConvertHashToRandom)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenConvertHashToRandom), m_fConvertHashToRandom);
	}

	// serialize columns
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenColumns));
	for (ULONG ul = 0; ul < m_pdrgpmdcol->Size(); ul++)
	{
		CMDColumn *pmdcol = (*m_pdrgpmdcol)[ul];
		pmdcol->Serialize(xml_serializer);
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

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenRelationExternal));
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDRelationExternalGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDRelationExternalGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "External Relation id: ";
	MDId()->OsPrint(os);
	os << std::endl;

	os << "Relation name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;

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

	os << "Reject limit: " << m_iRejectLimit;
	if (m_fRejLimitInRows)
	{
		os << " Rows" << std::endl;
	}
	else
	{
		os << " Percent" << std::endl;
	}

	if (NULL != PmdidFmtErrRel())
	{
		os << "Format Error Table: ";
		PmdidFmtErrRel()->OsPrint(os);
		os << std::endl;
	}
}

#endif // GPOS_DEBUG

// EOF

