//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLPhysicalCTAS.cpp
//
//	@doc:
//		Implementation of DXL physical CTAS operator
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLPhysicalCTAS.h"
#include "naucrates/dxl/operators/CDXLCtasStorageOptions.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/dxltokens.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTAS::CDXLPhysicalCTAS
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalCTAS::CDXLPhysicalCTAS
	(
	IMemoryPool *memory_pool,
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
	CDXLPhysical(memory_pool), 
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
//		CDXLPhysicalCTAS::~CDXLPhysicalCTAS
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalCTAS::~CDXLPhysicalCTAS()
{
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
//		CDXLPhysicalCTAS::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalCTAS::GetDXLOperator() const
{
	return EdxlopPhysicalCTAS;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTAS::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalCTAS::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalCTAS);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTAS::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalCTAS::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	if (NULL != m_mdname_schema)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSchema), m_mdname_schema->GetMDName());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_pmdnameRel->GetMDName());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelTemporary), m_is_temp_table);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelHasOids), m_has_oids);
	GPOS_ASSERT(NULL != IMDRelation::PstrStorageType(m_rel_storage_type));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelStorageType), IMDRelation::PstrStorageType(m_rel_storage_type));

	// serialize distribution columns
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelDistrPolicy), IMDRelation::PstrDistrPolicy(m_rel_distr_policy));
	
	if (IMDRelation::EreldistrHash == m_rel_distr_policy)
	{
		GPOS_ASSERT(NULL != m_distr_column_pos_array);
		
		// serialize distribution columns
		CWStringDynamic *pstrDistrColumns = CDXLUtils::Serialize(m_memory_pool, m_distr_column_pos_array);
		GPOS_ASSERT(NULL != pstrDistrColumns);
		
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDistrColumns), pstrDistrColumns);
		GPOS_DELETE(pstrDistrColumns);
	}

	// serialize input columns
	CWStringDynamic *pstrCols = CDXLUtils::Serialize(m_memory_pool, m_src_colids_array);
	GPOS_ASSERT(NULL != pstrCols);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), pstrCols);
	GPOS_DELETE(pstrCols);
	
	// serialize vartypmod list
	CWStringDynamic *pstrVarTypeModList = CDXLUtils::Serialize(m_memory_pool, m_vartypemod_array);
	GPOS_ASSERT(NULL != pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenVarTypeModList), pstrVarTypeModList);
	GPOS_DELETE(pstrVarTypeModList);

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize column descriptors
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));
	
	const ULONG ulArity = m_col_descr_array->Size();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLColDescr *pdxlcd = (*m_col_descr_array)[ul];
		pdxlcd->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));

	m_pdxlctasopt->Serialize(xml_serializer);
	
	// serialize arguments
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalCTAS::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalCTAS::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(2 == pdxln->Arity());

	CDXLNode *pdxlnPrL = (*pdxln)[0];
	CDXLNode *child_dxlnode = (*pdxln)[1];
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnPrL->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
