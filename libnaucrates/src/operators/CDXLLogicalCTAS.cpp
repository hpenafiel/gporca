//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLLogicalCTAS.cpp
//
//	@doc:
//		Implementation of DXL logical "CREATE TABLE AS" (CTAS) operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLLogicalCTAS.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::CDXLLogicalCTAS
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalCTAS::CDXLLogicalCTAS
	(
	IMemoryPool *memory_pool,
	IMDId *pmdid,
	CMDName *pmdnameSchema, 
	CMDName *pmdnameRel, 
	ColumnDescrDXLArray *pdrgpdxlcd,
	CDXLCtasStorageOptions *pdxlctasopt,
	IMDRelation::Ereldistrpolicy rel_distr_policy,
	ULongPtrArray *pdrgpulDistr,
	BOOL fTemporary,
	BOOL fHasOids, 
	IMDRelation::Erelstoragetype rel_storage_type,
	ULongPtrArray *pdrgpulSource,
	IntPtrArray *pdrgpiVarTypeMod
	)
	:
	CDXLLogical(memory_pool), 
	m_mdid(pmdid),
	m_mdname_schema(pmdnameSchema),
	m_pmdnameRel(pmdnameRel),
	m_col_descr_array(pdrgpdxlcd),
	m_pdxlctasopt(pdxlctasopt),
	m_rel_distr_policy(rel_distr_policy),
	m_distr_column_pos_array(pdrgpulDistr),
	m_is_temp_table(fTemporary),
	m_has_oids(fHasOids),
	m_rel_storage_type(rel_storage_type),
	m_src_colids_array(pdrgpulSource),
	m_vartypemod_array(pdrgpiVarTypeMod)
{
	GPOS_ASSERT(NULL != pmdid && pmdid->IsValid());
	GPOS_ASSERT(NULL != pmdnameRel);
	GPOS_ASSERT(NULL != pdrgpdxlcd);
	GPOS_ASSERT(NULL != pdxlctasopt);
	GPOS_ASSERT_IFF(IMDRelation::EreldistrHash == rel_distr_policy, NULL != pdrgpulDistr);
	GPOS_ASSERT(NULL != pdrgpulSource);
	GPOS_ASSERT(NULL != pdrgpiVarTypeMod);
	GPOS_ASSERT(pdrgpdxlcd->Size() == pdrgpiVarTypeMod->Size());
	GPOS_ASSERT(IMDRelation::ErelstorageSentinel > rel_storage_type);
	GPOS_ASSERT(IMDRelation::EreldistrSentinel > rel_distr_policy);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::~CDXLLogicalCTAS
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalCTAS::~CDXLLogicalCTAS()
{
	m_mdid->Release();
	GPOS_DELETE(m_mdname_schema);
	GPOS_DELETE(m_pmdnameRel);
	m_col_descr_array->Release();
	m_pdxlctasopt->Release();
	CRefCount::SafeRelease(m_distr_column_pos_array);
	m_src_colids_array->Release();
	m_vartypemod_array->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalCTAS::GetDXLOperator() const
{
	return EdxlopLogicalCTAS;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalCTAS::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenLogicalCTAS);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::IsColDefined
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalCTAS::IsColDefined
	(
	ULONG col_id
	)
	const
{
	const ULONG ulSize = m_col_descr_array->Size();
	for (ULONG ulDescr = 0; ulDescr < ulSize; ulDescr++)
	{
		ULONG ulId = (*m_col_descr_array)[ulDescr]->Id();
		if (ulId == col_id)
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTAS::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenMdid));
	if (NULL != m_mdname_schema)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenSchema), m_mdname_schema->GetMDName());
	}
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenName), m_pmdnameRel->GetMDName());

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelTemporary), m_is_temp_table);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelHasOids), m_has_oids);
	
	GPOS_ASSERT(NULL != IMDRelation::PstrStorageType(m_rel_storage_type));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelStorageType), IMDRelation::PstrStorageType(m_rel_storage_type));

	// serialize distribution columns
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenRelDistrPolicy), IMDRelation::PstrDistrPolicy(m_rel_distr_policy));
	
	if (IMDRelation::EreldistrHash == m_rel_distr_policy)
	{
		GPOS_ASSERT(NULL != m_distr_column_pos_array);
		
		// serialize distribution columns
		CWStringDynamic *pstrDistrColumns = CDXLUtils::Serialize(m_memory_pool, m_distr_column_pos_array);
		GPOS_ASSERT(NULL != pstrDistrColumns);
		
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenDistrColumns), pstrDistrColumns);
		GPOS_DELETE(pstrDistrColumns);
	}

	// serialize input columns
	CWStringDynamic *pstrCols = CDXLUtils::Serialize(m_memory_pool, m_src_colids_array);
	GPOS_ASSERT(NULL != pstrCols);

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenInsertCols), pstrCols);
	GPOS_DELETE(pstrCols);
	
	// serialize vartypmod list
	CWStringDynamic *pstrVarTypeModList = CDXLUtils::Serialize(m_memory_pool, m_vartypemod_array);
	GPOS_ASSERT(NULL != pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenVarTypeModList), pstrVarTypeModList);
	GPOS_DELETE(pstrVarTypeModList);

	// serialize column descriptors
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenColumns));
	
	const ULONG arity = m_col_descr_array->Size();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CDXLColDescr *pdxlcd = (*m_col_descr_array)[ul];
		pdxlcd->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), CDXLTokens::GetDXLTokenStr(EdxltokenColumns));

	m_pdxlctasopt->Serialize(xml_serializer);
	
	// serialize arguments
	dxlnode->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalCTAS::AssertValid
	(
	const CDXLNode *dxlnode,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(1 == dxlnode->Arity());

	CDXLNode *child_dxlnode = (*dxlnode)[0];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
