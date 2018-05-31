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
//		CDXLScalarArrayRef::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarArrayRef::Edxlop() const
{
	return EdxlopScalarArrayRef;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayRef::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayRef::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarArrayRef);
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
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_pmdidElem->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenArrayElementType));
	if (IDefaultTypeModifier != TypeModifier())
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenTypeMod), TypeModifier());
	}
	m_pmdidArray->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenArrayType));
	m_pmdidReturn->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenTypeId));

	// serialize child nodes
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(3 == ulArity || 4 == ulArity);

	// first 2 children are index lists
	(*pdxln)[0]->SerializeToDXL(xml_serializer);
	(*pdxln)[1]->SerializeToDXL(xml_serializer);

	// 3rd child is the ref expression
	const CWStringConst *pstrRefExpr = CDXLTokens::PstrToken(EdxltokenScalarArrayRefExpr);
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrRefExpr);
	(*pdxln)[2]->SerializeToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrRefExpr);

	// 4th child is the optional assign expression
	const CWStringConst *pstrAssignExpr = CDXLTokens::PstrToken(EdxltokenScalarArrayRefAssignExpr);
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrAssignExpr);
	if (4 == ulArity)
	{
		(*pdxln)[3]->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrAssignExpr);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
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
	CMDAccessor *pmda
	)
	const
{
	return (IMDType::EtiBool == pmda->Pmdtype(m_pmdidReturn)->Eti());
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
	BOOL fValidateChildren
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnChild = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnChild->Pdxlop()->Edxloperatortype());

		if (fValidateChildren)
		{
			pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
