//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarArrayComp.cpp
//
//	@doc:
//		Implementation of DXL scalar array comparison
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarArrayComp.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::CDXLScalarArrayComp
//
//	@doc:
//		Constructs a ScalarArrayComp node
//
//---------------------------------------------------------------------------
CDXLScalarArrayComp::CDXLScalarArrayComp
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidOp,
	const CWStringConst *pstrOpName,
	EdxlArrayCompType edxlcomptype
	)
	:
	CDXLScalarComp(memory_pool, pmdidOp, pstrOpName),
	m_edxlcomptype(edxlcomptype)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarArrayComp::GetDXLOperator() const
{
	return EdxlopScalarArrayComp;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::Edxlarraycomptype
//
//	@doc:
//	 	Returns the array comparison operation type (ALL/ANY)
//
//---------------------------------------------------------------------------
EdxlArrayCompType
CDXLScalarArrayComp::Edxlarraycomptype() const
{
	return m_edxlcomptype;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::PstrArrayCompType
//
//	@doc:
//		AggRef AggStage
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayComp::PstrArrayCompType() const
{
	switch (m_edxlcomptype)
	{
		case Edxlarraycomptypeany:
			return CDXLTokens::GetDXLTokenStr(EdxltokenOpTypeAny);
		case Edxlarraycomptypeall:
			return CDXLTokens::GetDXLTokenStr(EdxltokenOpTypeAll);
		default:
			GPOS_ASSERT(!"Unrecognized array operation type");
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarArrayComp::GetOpNameStr() const
{
	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarArrayComp);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayComp::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOpName), m_comparison_operator_name);
	m_mdid->Serialize(xml_serializer, CDXLTokens::GetDXLTokenStr(EdxltokenOpNo));
	xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenOpType), PstrArrayCompType());

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarArrayComp::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarArrayComp::AssertValid
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
		CDXLNode *dxlnode_arg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == dxlnode_arg->GetOperator()->GetDXLOperatorType());
		
		if (validate_children)
		{
			dxlnode_arg->GetOperator()->AssertValid(dxlnode_arg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG


// EOF
