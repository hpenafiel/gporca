//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarBoolExpr.cpp
//
//	@doc:
//		Implementation of DXL BoolExpr
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarBoolExpr.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBoolExpr::CDXLScalarBoolExpr
//
//	@doc:
//		Constructs a BoolExpr node
//
//---------------------------------------------------------------------------
CDXLScalarBoolExpr::CDXLScalarBoolExpr
	(
	IMemoryPool *memory_pool,
	const EdxlBoolExprType boolexptype
	)
	:
	CDXLScalar(memory_pool),
	m_boolexptype(boolexptype)
{

}



//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBoolExpr::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarBoolExpr::Edxlop() const
{
	return EdxlopScalarBoolExpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBoolExpr::EdxlBoolType
//
//	@doc:
//		Boolean expression type
//
//---------------------------------------------------------------------------
EdxlBoolExprType
CDXLScalarBoolExpr::EdxlBoolType() const
{
	return m_boolexptype;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBoolExpr::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarBoolExpr::PstrOpName() const
{
	switch (m_boolexptype)
	{
		case Edxland:
				return CDXLTokens::PstrToken(EdxltokenScalarBoolAnd);
		case Edxlor:
				return CDXLTokens::PstrToken(EdxltokenScalarBoolOr);
		case Edxlnot:
				return CDXLTokens::PstrToken(EdxltokenScalarBoolNot);
		default:
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBoolExpr::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarBoolExpr::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	GPOS_CHECK_ABORT;

	const CWStringConst *element_name = PstrOpName();

	GPOS_ASSERT(NULL != element_name);
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	GPOS_CHECK_ABORT;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarBoolExpr::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarBoolExpr::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	EdxlBoolExprType edxlbooltype = ((CDXLScalarBoolExpr *) pdxln->GetOperator())->EdxlBoolType();

	GPOS_ASSERT( (edxlbooltype == Edxlnot) || (edxlbooltype == Edxlor) || (edxlbooltype == Edxland));

	const ULONG ulArity = pdxln->Arity();
	if(edxlbooltype == Edxlnot)
	{
		GPOS_ASSERT(1 == ulArity);
	}
	else
	{
		GPOS_ASSERT(2 <= ulArity);
	}

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *pdxlnArg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == pdxlnArg->GetOperator()->Edxloperatortype());
		
		if (validate_children)
		{
			pdxlnArg->GetOperator()->AssertValid(pdxlnArg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
