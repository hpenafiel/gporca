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
	IMemoryPool *pmp,
	IMDId *pmdid,
	CMDName *pmdnameSchema, 
	CMDName *pmdnameRel, 
	ColumnDescrDXLArray *pdrgpdxlcd,
	CDXLCtasStorageOptions *pdxlctasopt,
	IMDRelation::Ereldistrpolicy ereldistrpolicy,
	ULongPtrArray *pdrgpulDistr,
	BOOL fTemporary,
	BOOL fHasOids, 
	IMDRelation::Erelstoragetype erelstorage,
	ULongPtrArray *pdrgpulSource,
	IntPtrArray *pdrgpiVarTypeMod
	)
	:
	CDXLLogical(pmp), 
	m_pmdid(pmdid),
	m_pmdnameSchema(pmdnameSchema),
	m_pmdnameRel(pmdnameRel),
	m_pdrgpdxlcd(pdrgpdxlcd),
	m_pdxlctasopt(pdxlctasopt),
	m_ereldistrpolicy(ereldistrpolicy),
	m_pdrgpulDistr(pdrgpulDistr),
	m_fTemporary(fTemporary),
	m_fHasOids(fHasOids),
	m_erelstorage(erelstorage),
	m_pdrgpulSource(pdrgpulSource),
	m_pdrgpiVarTypeMod(pdrgpiVarTypeMod)
{
	GPOS_ASSERT(NULL != pmdid && pmdid->IsValid());
	GPOS_ASSERT(NULL != pmdnameRel);
	GPOS_ASSERT(NULL != pdrgpdxlcd);
	GPOS_ASSERT(NULL != pdxlctasopt);
	GPOS_ASSERT_IFF(IMDRelation::EreldistrHash == ereldistrpolicy, NULL != pdrgpulDistr);
	GPOS_ASSERT(NULL != pdrgpulSource);
	GPOS_ASSERT(NULL != pdrgpiVarTypeMod);
	GPOS_ASSERT(pdrgpdxlcd->Size() == pdrgpiVarTypeMod->Size());
	GPOS_ASSERT(IMDRelation::ErelstorageSentinel > erelstorage);
	GPOS_ASSERT(IMDRelation::EreldistrSentinel > ereldistrpolicy);
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
	m_pmdid->Release();
	GPOS_DELETE(m_pmdnameSchema);
	GPOS_DELETE(m_pmdnameRel);
	m_pdrgpdxlcd->Release();
	m_pdxlctasopt->Release();
	CRefCount::SafeRelease(m_pdrgpulDistr);
	m_pdrgpulSource->Release();
	m_pdrgpiVarTypeMod->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalCTAS::Edxlop() const
{
	return EdxlopLogicalCTAS;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalCTAS::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalCTAS);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalCTAS::FDefinesColumn
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalCTAS::FDefinesColumn
	(
	ULONG ulColId
	)
	const
{
	const ULONG ulSize = m_pdrgpdxlcd->Size();
	for (ULONG ulDescr = 0; ulDescr < ulSize; ulDescr++)
	{
		ULONG ulId = (*m_pdrgpdxlcd)[ulDescr]->Id();
		if (ulId == ulColId)
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
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
	m_pmdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenMdid));
	if (NULL != m_pmdnameSchema)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSchema), m_pmdnameSchema->Pstr());
	}
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenName), m_pmdnameRel->Pstr());

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelTemporary), m_fTemporary);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelHasOids), m_fHasOids);
	
	GPOS_ASSERT(NULL != IMDRelation::PstrStorageType(m_erelstorage));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelStorageType), IMDRelation::PstrStorageType(m_erelstorage));

	// serialize distribution columns
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenRelDistrPolicy), IMDRelation::PstrDistrPolicy(m_ereldistrpolicy));
	
	if (IMDRelation::EreldistrHash == m_ereldistrpolicy)
	{
		GPOS_ASSERT(NULL != m_pdrgpulDistr);
		
		// serialize distribution columns
		CWStringDynamic *pstrDistrColumns = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulDistr);
		GPOS_ASSERT(NULL != pstrDistrColumns);
		
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenDistrColumns), pstrDistrColumns);
		GPOS_DELETE(pstrDistrColumns);
	}

	// serialize input columns
	CWStringDynamic *pstrCols = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulSource);
	GPOS_ASSERT(NULL != pstrCols);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInsertCols), pstrCols);
	GPOS_DELETE(pstrCols);
	
	// serialize vartypmod list
	CWStringDynamic *pstrVarTypeModList = CDXLUtils::Serialize(m_memory_pool, m_pdrgpiVarTypeMod);
	GPOS_ASSERT(NULL != pstrVarTypeModList);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenVarTypeModList), pstrVarTypeModList);
	GPOS_DELETE(pstrVarTypeModList);

	// serialize column descriptors
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));
	
	const ULONG ulArity = m_pdrgpdxlcd->Size();
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CDXLColDescr *pdxlcd = (*m_pdrgpdxlcd)[ul];
		pdxlcd->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));

	m_pdxlctasopt->Serialize(xml_serializer);
	
	// serialize arguments
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
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
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) 
	const
{
	GPOS_ASSERT(1 == pdxln->UlArity());

	CDXLNode *pdxlnChild = (*pdxln)[0];
	GPOS_ASSERT(EdxloptypeLogical == pdxlnChild->Pdxlop()->Edxloperatortype());

	if (fValidateChildren)
	{
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}

#endif // GPOS_DEBUG


// EOF
