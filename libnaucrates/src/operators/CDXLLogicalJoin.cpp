//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLLogicalJoin.cpp
//
//	@doc:
//		Implementation of DXL logical Join operator
//		
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLLogicalJoin.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::CDXLLogicalJoin
//
//	@doc:
//		Construct a DXL Logical Join node
//
//---------------------------------------------------------------------------
CDXLLogicalJoin::CDXLLogicalJoin
	(
	IMemoryPool *memory_pool,
	EdxlJoinType edxljt
	)
	:CDXLLogical(memory_pool),
	 m_edxljt(edxljt)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalJoin::Edxlop() const
{
	return EdxlopLogicalJoin;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::Edxltype
//
//	@doc:
//		Join type
//
//---------------------------------------------------------------------------
EdxlJoinType
CDXLLogicalJoin::Edxltype() const
{
	return m_edxljt;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalJoin::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalJoin);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalJoin::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinType), PstrJoinTypeName());

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::PstrJoinTypeName
//
//	@doc:
//		Join type name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalJoin::PstrJoinTypeName() const
{
	return CDXLOperator::PstrJoinTypeName(m_edxljt);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalJoin::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalJoin::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) const
{
	const ULONG ulChildren = pdxln->Arity();
	GPOS_ASSERT(2 < ulChildren);

	for (ULONG ul = 0; ul < ulChildren - 1; ++ul)
	{
		CDXLNode *pdxlnChild = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeLogical == pdxlnChild->Pdxlop()->Edxloperatortype());
		
		if (fValidateChildren)
		{
			pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
		}
	}

	CDXLNode *pdxlnLastChild = (*pdxln)[ulChildren - 1];
	GPOS_ASSERT(NULL != pdxlnLastChild);

	//The last child is a CDXLScalar operator representing the join qual
	GPOS_ASSERT(EdxloptypeScalar == pdxlnLastChild->Pdxlop()->Edxloperatortype());
	
	if (fValidateChildren)
	{
		pdxlnLastChild->Pdxlop()->AssertValid(pdxlnLastChild, fValidateChildren);
	}
}
#endif // GPOS_DEBUG

// EOF
