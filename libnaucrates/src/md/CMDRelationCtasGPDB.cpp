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
	IMDId *mdid,
	CMDName *mdname_schema,
	CMDName *mdname,
	BOOL fTemporary,
	BOOL fHasOids,
	Erelstoragetype rel_storage_type,
	GetRelDistrPolicy rel_distr_policy,
	DrgPmdcol *pdrgpmdcol,
	ULongPtrArray *pdrgpulDistrColumns,
	ULongPtrArray2D *pdrgpdrgpulKeys,
	CDXLCtasStorageOptions *dxl_ctas_storage_options,
	IntPtrArray *pdrgpiVarTypeMod
	)
	:
	m_memory_pool(memory_pool),
	m_mdid(mdid),
	m_mdname_schema(mdname_schema),
	m_mdname(mdname),
	m_is_temp_table(fTemporary),
	m_has_oids(fHasOids),
	m_rel_storage_type(rel_storage_type),
	m_rel_distr_policy(rel_distr_policy),
	m_md_col_array(pdrgpmdcol),
	m_distr_col_array(pdrgpulDistrColumns),
	m_pdrgpdrgpulKeys(pdrgpdrgpulKeys),
	m_ulSystemColumns(0),
	m_pdrgpulNonDroppedCols(NULL),
	m_dxl_ctas_storage_option(dxl_ctas_storage_options),
	m_vartypemod_array(pdrgpiVarTypeMod)
{
	GPOS_ASSERT(mdid->IsValid());
	GPOS_ASSERT(NULL != pdrgpmdcol);
	GPOS_ASSERT(NULL != dxl_ctas_storage_options);
	GPOS_ASSERT(IMDRelation::ErelstorageSentinel > m_rel_storage_type);	
	GPOS_ASSERT(0 == pdrgpdrgpulKeys->Size());
	GPOS_ASSERT(NULL != pdrgpiVarTypeMod);
	
	m_phmiulAttno2Pos = GPOS_NEW(m_memory_pool) HMIUl(m_memory_pool);
	m_pdrgpulNonDroppedCols = GPOS_NEW(m_memory_pool) ULongPtrArray(m_memory_pool);
	m_pdrgpdoubleColWidths = GPOS_NEW(memory_pool) DrgPdouble(memory_pool);

	const ULONG arity = pdrgpmdcol->Size();
	for (ULONG ul = 0; ul < arity; ul++)
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
	m_md_col_array->Release();
	m_pdrgpdrgpulKeys->Release();
	m_pdrgpdoubleColWidths->Release();
	CRefCount::SafeRelease(m_distr_col_array);
	CRefCount::SafeRelease(m_phmiulAttno2Pos);
	CRefCount::SafeRelease(m_pdrgpulNonDroppedCols);
	m_dxl_ctas_storage_option->Release();
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
//		CMDRelationCtasGPDB::GetMdNameSchema
//
//	@doc:
//		Returns schema name
//
//---------------------------------------------------------------------------
CMDName *
CMDRelationCtasGPDB::GetMdNameSchema() const
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
IMDRelation::GetRelDistrPolicy
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
	GPOS_ASSERT(NULL != m_md_col_array);

	return m_md_col_array->Size();
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
	INT attno
	)
	const
{
	ULONG *pul = m_phmiulAttno2Pos->Find(&attno);
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
	return (m_distr_col_array == NULL) ? 0 : m_distr_col_array->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDRelationCtasGPDB::GetMdCol
//
//	@doc:
//		Returns the column at the specified position
//
//---------------------------------------------------------------------------
const IMDColumn *
CMDRelationCtasGPDB::GetMdCol
	(
	ULONG ulPos
	)
	const
{
	GPOS_ASSERT(ulPos < m_md_col_array->Size());

	return (*m_md_col_array)[ulPos];
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
	GPOS_ASSERT(ulPos < m_distr_col_array->Size());

	ULONG ulDistrKeyPos = (*(*m_distr_col_array)[ulPos]);
	return GetMdCol(ulDistrKeyPos);
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
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenRelationCTAS));

	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	if (NULL != m_mdname_schema)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenSchema), m_mdname_schema->GetMDName());
	}
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_mdname->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelTemporary), m_is_temp_table);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelHasOids), m_has_oids);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageType), IMDRelation::PstrStorageType(m_rel_storage_type));

	// serialize vartypmod list
	CWStringDynamic *pstrVarTypeModList = CDXLUtils::Serialize(m_memory_pool, m_vartypemod_array);
	GPOS_ASSERT(NULL != pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenVarTypeModList), pstrVarTypeModList);
	GPOS_DELETE(pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelDistrPolicy), PstrDistrPolicy(m_rel_distr_policy));

	if (EreldistrHash == m_rel_distr_policy)
	{
		GPOS_ASSERT(NULL != m_distr_col_array);

		// serialize distribution columns
		CWStringDynamic *pstrDistrColumns = PstrColumns(m_memory_pool, m_distr_col_array);
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDistrColumns), pstrDistrColumns);
		GPOS_DELETE(pstrDistrColumns);
	}

	// serialize columns
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenColumns));
	const ULONG ulCols = m_md_col_array->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		CMDColumn *pmdcol = (*m_md_col_array)[ul];
		pmdcol->Serialize(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenColumns));

	m_dxl_ctas_storage_option->Serialize(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
						CDXLTokens::GetDXLTokenStr(EdxltokenRelationCTAS));
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

	os << "Relation name: " << (Mdname()).GetMDName()->GetBuffer() << std::endl;

	os << "Distribution policy: " << PstrDistrPolicy(m_rel_distr_policy)->GetBuffer() << std::endl;

	os << "Relation columns: " << std::endl;
	const ULONG ulColumns = UlColumns();
	for (ULONG ul = 0; ul < ulColumns; ul++)
	{
		const IMDColumn *pimdcol = GetMdCol(ul);
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
}

#endif // GPOS_DEBUG

// EOF

