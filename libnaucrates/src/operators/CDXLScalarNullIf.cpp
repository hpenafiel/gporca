//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarNullIf.cpp
//
//	@doc:
//		Implementation of DXL NullIf operator
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarNullIf.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::CDXLScalarNullIf
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarNullIf::CDXLScalarNullIf
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	IMDId *mdid_type
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidOp(pmdidOp),
	m_mdid_type(mdid_type)
{
	GPOS_ASSERT(pmdidOp->IsValid());
	GPOS_ASSERT(mdid_type->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::~CDXLScalarNullIf
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarNullIf::~CDXLScalarNullIf()
{
	m_pmdidOp->Release();
	m_mdid_type->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarNullIf::GetDXLOperator() const
{
	return EdxlopScalarNullIf;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::PmdidOp
//
//	@doc:
//		Operator id
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarNullIf::PmdidOp() const
{
	return m_pmdidOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::MDIdType
//
//	@doc:
//		Return type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarNullIf::MDIdType() const
{
	return m_mdid_type;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarNullIf::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarNullIf);;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::HasBoolResult
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarNullIf::HasBoolResult
	(
	CMDAccessor *md_accessor
	)
	const
{
	return (IMDType::EtiBool == md_accessor->Pmdtype(m_mdid_type)->Eti());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarNullIf::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	m_pmdidOp->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenOpNo));
	m_mdid_type->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarNullIf::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarNullIf::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG arity = pdxln->Arity();
	GPOS_ASSERT(2 == arity);

	for (ULONG ul = 0; ul < arity; ++ul)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == child_dxlnode->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
