//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CMDRelationCtasGPDB.cpp
//
//	@doc:
//		Implementation of the class for representing MD cache CTAS relations
//---------------------------------------------------------------------------

#include "naucrates/md/CMDRelationCtasGPDB.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"

#include "naucrates/dxl/CDXLUtils.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::CMDRelationCtasGPDB
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDRelationCtasGPDB::CMDRelationCtasGPDB
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *pmdnameSchema,
	CMDName *mdname,
	BOOL fTemporary,
	BOOL fHasOids,
	Erelstoragetype rel_storage_type,
	Ereldistrpolicy rel_distr_policy,
	DrgPmdcol *pdrgpmdcol,
	ULongPtrArray *pdrgpulDistrColumns,
	ULongPtrArray2D *pdrgpdrgpulKeys,
	CDXLCtasStorageOptions *pdxlctasopt,
	IntPtrArray *pdrgpiVarTypeMod
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(pmdid),
	m_mdname_schema(pmdnameSchema),
	m_mdname(mdname),
	m_is_temp_table(fTemporary),
	m_has_oids(fHasOids),
	m_rel_storage_type(rel_storage_type),
	m_rel_distr_policy(rel_distr_policy),
	m_pdrgpmdcol(pdrgpmdcol),
	m_pdrgpulDistrColumns(pdrgpulDistrColumns),
	m_pdrgpdrgpulKeys(pdrgpdrgpulKeys),
	m_ulSystemColumns(0),
	m_pdrgpulNonDroppedCols(NULL),
	m_pdxlctasopt(pdxlctasopt),
	m_vartypemod_array(pdrgpiVarTypeMod)
{
	GPOS_ASSERT(pmdid->IsValid());
	GPOS_ASSERT(NULL != pdrgpmdcol);
	GPOS_ASSERT(NULL != pdxlctasopt);
	GPOS_ASSERT(IMDRelation::ErelstorageSentinel > m_rel_storage_type);	
	GPOS_ASSERT(0 == pdrgpdrgpulKeys->Size());
	GPOS_ASSERT(NULL != pdrgpiVarTypeMod);
	
	m_phmiulAttno2Pos = GPOS_NEW(m_memory_pool) HMIUl(m_memory_pool);
	m_pdrgpulNonDroppedCols = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	m_pdrgpdoubleColWidths = GPOS_NEW(memory_pool) DrgPdouble(memory_pool);

	const ULONG ulArity = pdrgpmdcol->Size();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		IMDColumn *pmdcol = (*pdrgpmdcol)[ul];
		GPOS_ASSERT(!pmdcol->IsDropped() && "Cannot create a table with dropped columns");

		BOOL fSystemCol = pmdcol->FSystemColumn();
		if (fSystemCol)
		{
			m_ulSystemColumns++;
		}
		else
		{
			m_pdrgpulNonDroppedCols->Append(GPOS_NEW(m_memory_pool) ULONG(ul));
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
//		CMDRelationCtasGPDB::~CMDRelationCtasGPDB
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDRelationCtasGPDB::~CMDRelationCtasGPDB()
{
	GPOS_DELETE(m_mdname_schema);
	GPOS_DELETE(m_mdname);
	GPOS_DELETE(m_pstr);
	m_mdid->Release();
	m_pdrgpmdcol->Release();
	m_pdrgpdrgpulKeys->Release();
	m_pdrgpdoubleColWidths->Release();
	CRefCount::SafeRelease(m_pdrgpulDistrColumns);
	CRefCount::SafeRelease(m_phmiulAttno2Pos);
	CRefCount::SafeRelease(m_pdrgpulNonDroppedCols);
	m_pdxlctasopt->Release();
	m_vartypemod_array->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::MDId
//
//	@doc:
//		Returns the metadata id of this relation
//
//---------------------------------------------------------------------------
IMDId *
CMDRelationCtasGPDB::MDId() const
{
	return m_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::Mdname
//
//	@doc:
//		Returns the name of this relation
//
//---------------------------------------------------------------------------
CMDName
CMDRelationCtasGPDB::Mdname() const
{
	return *m_mdname;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::PmdnameSchema
//
//	@doc:
//		Returns schema name
//
//---------------------------------------------------------------------------
CMDName *
CMDRelationCtasGPDB::PmdnameSchema() const
{
	return m_mdname_schema;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::Ereldistribution
//
//	@doc:
//		Returns the distribution policy for this relation
//
//---------------------------------------------------------------------------
IMDRelation::Ereldistrpolicy
CMDRelationCtasGPDB::Ereldistribution() const
{
	return m_rel_distr_policy;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::UlColumns
//
//	@doc:
//		Returns the number of columns of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationCtasGPDB::UlColumns() const
{
	GPOS_ASSERT(NULL != m_pdrgpmdcol);

	return m_pdrgpmdcol->Size();
}

// Return the width of a column with regards to the position
DOUBLE
CMDRelationCtasGPDB::DColWidth
	(
	ULONG ulPos
	)
const
{
	return (*m_pdrgpdoubleColWidths)[ulPos]->Get();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::UlSystemColumns
//
//	@doc:
//		Returns the number of system columns of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationCtasGPDB::UlSystemColumns() const
{
	return m_ulSystemColumns;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::UlPosFromAttno
//
//	@doc:
//		Return the position of a column in the metadata object given the
//		attribute number in the system catalog
//---------------------------------------------------------------------------
ULONG
CMDRelationCtasGPDB::UlPosFromAttno
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
//		CMDRelationCtasGPDB::UlDistrColumns
//
//	@doc:
//		Returns the number of columns in the distribution column list of this relation
//
//---------------------------------------------------------------------------
ULONG
CMDRelationCtasGPDB::UlDistrColumns() const
{
	return (m_pdrgpulDistrColumns == NULL) ? 0 : m_pdrgpulDistrColumns->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::Pmdcol
//
//	@doc:
//		Returns the column at the specified position
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationCtasGPDB::Pmdcol
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
//		CMDRelationCtasGPDB::PmdcolDistrColumn
//
//	@doc:
//		Returns the distribution column at the specified position in the distribution column list
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationCtasGPDB::PmdcolDistrColumn
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
//		CMDRelationCtasGPDB::Serialize
//
//	@doc:
//		Serialize relation metadata in DXL format
//
//---------------------------------------------------------------------------
void
CMDRelationCtasGPDB::Serialize
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenRelationCTAS));

	m_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	if (NULL != m_mdname_schema)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSchema), m_mdname_schema->Pstr());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_mdname->Pstr());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelTemporary), m_is_temp_table);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelHasOids), m_has_oids);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelStorageType), IMDRelation::PstrStorageType(m_rel_storage_type));

	// serialize vartypmod list
	CWStringDynamic *pstrVarTypeModList = CDXLUtils::Serialize(m_memory_pool, m_vartypemod_array);
	GPOS_ASSERT(NULL != pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenVarTypeModList), pstrVarTypeModList);
	GPOS_DELETE(pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelDistrPolicy), PstrDistrPolicy(m_rel_distr_policy));

	if (EreldistrHash == m_rel_distr_policy)
	{
		GPOS_ASSERT(NULL != m_pdrgpulDistrColumns);

		// serialize distribution columns
		CWStringDynamic *pstrDistrColumns = PstrColumns(m_memory_pool, m_pdrgpulDistrColumns);
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDistrColumns), pstrDistrColumns);
		GPOS_DELETE(pstrDistrColumns);
	}

	// serialize columns
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenColumns));
	const ULONG ulCols = m_pdrgpmdcol->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CMDColumn *pmdcol = (*m_pdrgpmdcol)[ul];
		pmdcol->Serialize(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenColumns));

	m_pdxlctasopt->Serialize(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						CDXLTokens::PstrToken(EdxltokenRelationCTAS));
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::DebugPrint
//
//	@doc:
//		Prints a metadata cache relation to the provided output
//
//---------------------------------------------------------------------------
void
CMDRelationCtasGPDB::DebugPrint
	(
	IOstream &os
	)
	const
{
	os << "CTAS Relation id: ";
	MDId()->OsPrint(os);
	os << std::endl;

	os << "Relation name: " << (Mdname()).Pstr()->GetBuffer() << std::endl;

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
		os << (pimdcolDistrKey->Mdname()).Pstr()->GetBuffer();
	}

	os << std::endl;
}

#endif // GPOS_DEBUG

// EOF

