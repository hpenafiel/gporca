//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalRowTrigger.cpp
//
//	@doc:
//		Implementation of DXL physical row trigger operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalRowTrigger.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/CDXLUtils.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRowTrigger::CDXLPhysicalRowTrigger
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalRowTrigger::CDXLPhysicalRowTrigger
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidRel,
	INT iType,
	ULongPtrArray *pdrgpulOld,
	ULongPtrArray *pdrgpulNew
	)
	:
	CDXLPhysical(memory_pool),
	m_rel_mdid(pmdidRel),
	m_iType(iType),
	m_pdrgpulOld(pdrgpulOld),
	m_pdrgpulNew(pdrgpulNew)
{
	GPOS_ASSERT(pmdidRel->IsValid());
	GPOS_ASSERT(0 != iType);
	GPOS_ASSERT(NULL != pdrgpulNew || NULL != pdrgpulOld);
	GPOS_ASSERT_IMP(NULL != pdrgpulNew && NULL != pdrgpulOld,
			pdrgpulNew->Size() == pdrgpulOld->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRowTrigger::~CDXLPhysicalRowTrigger
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalRowTrigger::~CDXLPhysicalRowTrigger()
{
	m_rel_mdid->Release();
	CRefCount::SafeRelease(m_pdrgpulOld);
	CRefCount::SafeRelease(m_pdrgpulNew);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRowTrigger::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalRowTrigger::GetDXLOperator() const
{
	return EdxlopPhysicalRowTrigger;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRowTrigger::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalRowTrigger::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalRowTrigger);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRowTrigger::SerializeToDXL
//
//	@doc:
//		Serialize function descriptor in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRowTrigger::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_rel_mdid->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenRelationMdid));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenMDType), m_iType);

	if (NULL != m_pdrgpulOld)
	{
		CWStringDynamic *pstrColsOld = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulOld);
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOldCols), pstrColsOld);
		GPOS_DELETE(pstrColsOld);
	}

	if (NULL != m_pdrgpulNew)
	{
		CWStringDynamic *pstrColsNew = CDXLUtils::Serialize(m_memory_pool, m_pdrgpulNew);
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenNewCols), pstrColsNew);
		GPOS_DELETE(pstrColsNew);
	}

	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize project list
	(*pdxln)[0]->SerializeToDXL(xml_serializer);

	// serialize physical child
	(*pdxln)[1]->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalRowTrigger::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalRowTrigger::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(2 == pdxln->Arity());
	CDXLNode *child_dxlnode = (*pdxln)[1];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}

#endif // GPOS_DEBUG


// EOF
