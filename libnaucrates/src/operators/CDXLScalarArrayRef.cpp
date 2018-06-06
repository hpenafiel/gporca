//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarArrayRef.cpp
//
//	@doc:
//		Implementation of DXL arrayrefs
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarArrayRef.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::CDXLScalarArrayRef
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarArrayRef::CDXLScalarArrayRef
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidElem,
	INT type_modifier,
	IMDId *pmdidArray,
	IMDId *pmdidReturn
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidElem(pmdidElem),
	m_type_modifier(type_modifier),
	m_pmdidArray(pmdidArray),
	m_pmdidReturn(pmdidReturn)
{
	GPOS_ASSERT(m_pmdidElem->IsValid());
	GPOS_ASSERT(m_pmdidArray->IsValid());
	GPOS_ASSERT(m_pmdidReturn->IsValid());
	GPOS_ASSERT(m_pmdidReturn->Equals(m_pmdidElem) || m_pmdidReturn->Equals(m_pmdidArray));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::~CDXLScalarArrayRef
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarArrayRef::~CDXLScalarArrayRef()
{
	m_pmdidElem->Release();
	m_pmdidArray->Release();
	m_pmdidReturn->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarArrayRef::GetDXLOperator() const
{
	return EdxlopScalarArrayRef;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayRef::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarArrayRef);
}

INT
CDXLScalarArrayRef::TypeModifier() const
{
	return m_type_modifier;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayRef::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_pmdidElem->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenArrayElementType));
	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenTypeMod), TypeModifier());
	}
	m_pmdidArray->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenArrayType));
	m_pmdidReturn->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenTypeId));

	// serialize child nodes
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(3 == ulArity || 4 == ulArity);

	// first 2 children are index lists
	(*pdxln)[0]->SerializeToDXL(xml_serializer);
	(*pdxln)[1]->SerializeToDXL(xml_serializer);

	// 3rd child is the ref expression
	const CWStringConst *pstrRefExpr = CDXLTokens::GetDXLTokenStr(EdxltokenScalarArrayRefExpr);
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrRefExpr);
	(*pdxln)[2]->SerializeToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrRefExpr);

	// 4th child is the optional assign expression
	const CWStringConst *pstrAssignExpr = CDXLTokens::GetDXLTokenStr(EdxltokenScalarArrayRefAssignExpr);
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrAssignExpr);
	if (4 == ulArity)
	{
		(*pdxln)[3]->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), pstrAssignExpr);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::FBoolean
//
//	@doc:
//		Does the operator return boolean result
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarArrayRef::FBoolean
	(
	CMDAccessor *md_accessor
	)
	const
{
	return (IMDType::EtiBool == md_accessor->Pmdtype(m_pmdidReturn)->Eti());
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayRef::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
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
