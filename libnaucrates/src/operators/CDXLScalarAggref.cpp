//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarAggref.cpp
//
//	@doc:
//		Implementation of DXL AggRef
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarAggref.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

#include "naucrates/md/IMDAggregate.h"

using namespace gpopt;
using namespace gpmd;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::CDXLScalarAggref
//
//	@doc:
//		Constructs an AggRef node
//
//---------------------------------------------------------------------------
CDXLScalarAggref::CDXLScalarAggref
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidAggOid,
	IMDId *pmdidResolvedRetType,
	BOOL fDistinct,
	EdxlAggrefStage edxlaggstage
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidAgg(pmdidAggOid),
	m_pmdidResolvedRetType(pmdidResolvedRetType),
	m_is_distinct(fDistinct),
	m_edxlaggstage(edxlaggstage)
{
	GPOS_ASSERT(NULL != pmdidAggOid);
	GPOS_ASSERT_IMP(NULL != pmdidResolvedRetType, pmdidResolvedRetType->IsValid());
	GPOS_ASSERT(m_pmdidAgg->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::~CDXLScalarAggref
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CDXLScalarAggref::~CDXLScalarAggref()
{
	m_pmdidAgg->Release();
	CRefCount::SafeRelease(m_pmdidResolvedRetType);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarAggref::GetDXLOperator() const
{
	return EdxlopScalarAggref;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::Edxlaggstage
//
//	@doc:
//		AggRef AggStage
//
//---------------------------------------------------------------------------
EdxlAggrefStage
CDXLScalarAggref::Edxlaggstage() const
{
	return m_edxlaggstage;
}
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::PstrAggStage
//
//	@doc:
//		AggRef AggStage
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarAggref::PstrAggStage() const
{
	switch (m_edxlaggstage)
	{
		case EdxlaggstageNormal:
			return CDXLTokens::GetDXLTokenStr(EdxltokenAggrefStageNormal);
		case EdxlaggstagePartial:
			return CDXLTokens::GetDXLTokenStr(EdxltokenAggrefStagePartial);
		case EdxlaggstageIntermediate:
			return CDXLTokens::GetDXLTokenStr(EdxltokenAggrefStageIntermediate);
		case EdxlaggstageFinal:
			return CDXLTokens::GetDXLTokenStr(EdxltokenAggrefStageFinal);
		default:
			GPOS_ASSERT(!"Unrecognized aggregate stage");
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarAggref::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarAggref);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::PmdidAgg
//
//	@doc:
//		Returns function id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarAggref::PmdidAgg() const
{
	return m_pmdidAgg;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::PmdidResolvedRetType
//
//	@doc:
//		Returns resolved type id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarAggref::PmdidResolvedRetType() const
{
	return m_pmdidResolvedRetType;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::IsDistinct
//
//	@doc:
//		TRUE if it's agg(DISTINCT ...)
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarAggref::IsDistinct() const
{
	return m_is_distinct;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarAggref::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *dxlnode
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_pmdidAgg->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenAggrefOid));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenAggrefDistinct),m_is_distinct);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenAggrefStage), PstrAggStage());
	if (NULL != m_pmdidResolvedRetType)
	{
		m_pmdidResolvedRetType->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));
	}
	dxlnode->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::HasBoolResult
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarAggref::HasBoolResult
	(
	CMDAccessor *md_accessor
	)
	const
{
	const IMDAggregate *pmdagg = md_accessor->Pmdagg(m_pmdidAgg);
	return (IMDType::EtiBool == md_accessor->Pmdtype(pmdagg->PmdidTypeResult())->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarAggref::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarAggref::AssertValid
	(
	const CDXLNode *dxlnode,
	BOOL validate_children
	) 
	const
{
	EdxlAggrefStage edxlaggrefstage = ((CDXLScalarAggref*) dxlnode->GetOperator())->Edxlaggstage();

	GPOS_ASSERT((EdxlaggstageFinal >= edxlaggrefstage) && (EdxlaggstageNormal <= edxlaggrefstage));

	const ULONG arity = dxlnode->Arity();
	for (ULONG ul = 0; ul < arity; ++ul)
	{
		CDXLNode *pdxlnAggrefArg = (*dxlnode)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnAggrefArg->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			pdxlnAggrefArg->GetOperator()->AssertValid(pdxlnAggrefArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
