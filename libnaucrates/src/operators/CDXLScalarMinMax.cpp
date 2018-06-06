//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 Pivotal Inc.
//
//	@filename:
//		CDXLScalarMinMax.cpp
//
//	@doc:
//		Implementation of DXL MinMax
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarMinMax.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpopt;
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::CDXLScalarMinMax
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarMinMax::CDXLScalarMinMax
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_type,
	EdxlMinMaxType emmt
	)
	:
	CDXLScalar(memory_pool),
	m_mdid_type(mdid_type),
	m_emmt(emmt)
{
	GPOS_ASSERT(m_mdid_type->IsValid());
	GPOS_ASSERT(EmmtSentinel > emmt);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::~CDXLScalarMinMax
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarMinMax::~CDXLScalarMinMax()
{
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarMinMax::GetDXLOperator() const
{
	return EdxlopScalarMinMax;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarMinMax::GetOpNameStr() const
{
	switch (m_emmt)
	{
		case EmmtMin:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarMin);
		case EmmtMax:
				return CDXLTokens::GetDXLTokenStr(EdxltokenScalarMax);
		default:
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarMinMax::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));
	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::FBoolean
//
//	@doc:
//		Does the operator return a boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarMinMax::FBoolean
	(
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_mdid_type)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarMinMax::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarMinMax::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(0 < pdxln->Arity());

	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
