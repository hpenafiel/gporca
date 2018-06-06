//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarSubPlan.cpp
//
//	@doc:
//		Implementation of DXL Scalar SubPlan operator
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/base/COptCtxt.h"

#include "naucrates/dxl/operators/CDXLScalarSubPlan.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpopt;
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::CDXLScalarSubPlan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarSubPlan::CDXLScalarSubPlan
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidFirstColType,
	DrgPdxlcr *pdrgdxlcr,
	EdxlSubPlanType edxlsubplantype,
	CDXLNode *pdxlnTestExpr
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidFirstColType(pmdidFirstColType),
	m_pdrgdxlcr(pdrgdxlcr),
	m_edxlsubplantype(edxlsubplantype),
	m_pdxlnTestExpr(pdxlnTestExpr)
{
	GPOS_ASSERT(EdxlSubPlanTypeSentinel > edxlsubplantype);
	GPOS_ASSERT_IMP(EdxlSubPlanTypeAny == edxlsubplantype || EdxlSubPlanTypeAll == edxlsubplantype, NULL != pdxlnTestExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::~CDXLScalarSubPlan
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarSubPlan::~CDXLScalarSubPlan()
{
	m_pmdidFirstColType->Release();
	m_pdrgdxlcr->Release();
	CRefCount::SafeRelease(m_pdxlnTestExpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubPlan::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlan);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::PmdidFirstColType
//
//	@doc:
//		Return type id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarSubPlan::PmdidFirstColType() const
{
	return m_pmdidFirstColType;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarSubPlan::FBoolean
	(
	CMDAccessor *md_accessor
	)
	const
{
	return (IMDType::EtiBool == md_accessor->Pmdtype(m_pmdidFirstColType)->Eti());
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::PstrSubplanType
//
//	@doc:
//		Return a string representation of Subplan type
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubPlan::PstrSubplanType() const
{
	switch (m_edxlsubplantype)
	{
		case EdxlSubPlanTypeScalar:
			return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTypeScalar);

		case EdxlSubPlanTypeExists:
			return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTypeExists);

		case EdxlSubPlanTypeNotExists:
			return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTypeNotExists);

		case EdxlSubPlanTypeAny:
			return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTypeAny);

		case EdxlSubPlanTypeAll:
			return CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTypeAll);

		default:
			GPOS_ASSERT(!"Unrecognized subplan type");
			return NULL;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSubPlan::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_pmdidFirstColType->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanType), PstrSubplanType());

	// serialize test expression
	xml_serializer->OpenElement
					(
					CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
					CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTestExpr)
					);

	if (NULL != m_pdxlnTestExpr)
	{
		m_pdxlnTestExpr->SerializeToDXL(xml_serializer);
	}

	xml_serializer->CloseElement
					(
					CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
					CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanTestExpr)
					);

	xml_serializer->OpenElement
				(
				CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
				CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanParamList)
				);

	for (ULONG ul = 0; ul < m_pdrgdxlcr->Size(); ul++)
	{
		xml_serializer->OpenElement
					(
					CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
					CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanParam)
					);

		ULONG ulid = (*m_pdrgdxlcr)[ul]->Id();
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColId), ulid);

		const CMDName *mdname = (*m_pdrgdxlcr)[ul]->MdName();
		const IMDId *mdid_type = (*m_pdrgdxlcr)[ul]->MDIdType();
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenColName), mdname->GetMDName());
		mdid_type->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));

		xml_serializer->CloseElement
					(
					CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
					CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanParam)
					);
	}

	xml_serializer->CloseElement
				(
				CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix),
				CDXLTokens::GetDXLTokenStr(EdxltokenScalarSubPlanParamList)
				);

	GPOS_ASSERT(1 == pdxln->GetChildDXLNodeArray()->Size());

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubPlan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarSubPlan::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(EdxlSubPlanIndexSentinel == pdxln->Arity());

	// assert child plan is a physical plan and is valid

	CDXLNode *child_dxlnode = (*pdxln)[EdxlSubPlanIndexChildPlan];
	GPOS_ASSERT(NULL != child_dxlnode);
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT_IMP(NULL != m_pdxlnTestExpr, EdxloptypeScalar == m_pdxlnTestExpr->GetOperator()->GetDXLOperatorType());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
