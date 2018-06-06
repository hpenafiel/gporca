//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarArray.cpp
//
//	@doc:
//		Implementation of DXL arrays
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarArray.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::CDXLScalarArray
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarArray::CDXLScalarArray
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidElem,
	IMDId *pmdidArray,
	BOOL fMultiDimensional
	)
	:
	CDXLScalar(memory_pool),
	m_pmdidElem(pmdidElem),
	m_pmdidArray(pmdidArray),
	m_fMultiDimensional(fMultiDimensional)
{
	GPOS_ASSERT(m_pmdidElem->IsValid());
	GPOS_ASSERT(m_pmdidArray->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::~CDXLScalarArray
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLScalarArray::~CDXLScalarArray()
{
	m_pmdidElem->Release();
	m_pmdidArray->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarArray::GetDXLOperator() const
{
	return EdxlopScalarArray;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArray::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarArray);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::PmdidElem
//
//	@doc:
//		Id of base element type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarArray::PmdidElem() const
{
	return m_pmdidElem;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::PmdidArray
//
//	@doc:
//		Id of array type
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarArray::PmdidArray() const
{
	return m_pmdidArray;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::FMultiDimensional
//
//	@doc:
//		Is this a multi-dimensional array
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarArray::FMultiDimensional() const
{
	return m_fMultiDimensional;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarArray::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	m_pmdidArray->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenArrayType));
	m_pmdidElem->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenArrayElementType));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenArrayMultiDim),m_fMultiDimensional);
	
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArray::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarArray::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	const ULONG arity = pdxln->Arity();
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
