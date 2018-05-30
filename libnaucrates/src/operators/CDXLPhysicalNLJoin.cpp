//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalNLJoin.cpp
//
//	@doc:
//		Implementation of DXL physical nested loop join operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalNLJoin.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::CDXLPhysicalNLJoin
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalNLJoin::CDXLPhysicalNLJoin
	(
	IMemoryPool *memory_pool,
	EdxlJoinType edxljt,
	BOOL fIndexNLJ
	)
	:
	CDXLPhysicalJoin(memory_pool, edxljt),
	m_fIndexNLJ(fIndexNLJ)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalNLJoin::Edxlop() const
{
	return EdxlopPhysicalNLJoin;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalNLJoin::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalNLJoin);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalNLJoin::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *pstrElemName = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);		
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenJoinType), PstrJoinTypeName());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPhysicalNLJoinIndex), m_fIndexNLJ);


	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);		
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalNLJoin::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalNLJoin::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL fValidateChildren
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, fValidateChildren);
	
	GPOS_ASSERT(EdxlnljIndexSentinel == pdxln->Arity());
	GPOS_ASSERT(EdxljtSentinel > Edxltype());
	
	CDXLNode *pdxlnJoinFilter = (*pdxln)[EdxlnljIndexJoinFilter];
	CDXLNode *pdxlnLeft = (*pdxln)[EdxlnljIndexLeftChild];
	CDXLNode *pdxlnRight = (*pdxln)[EdxlnljIndexRightChild];
	
	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxlopScalarJoinFilter == pdxlnJoinFilter->Pdxlop()->Edxlop());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnLeft->Pdxlop()->Edxloperatortype());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnRight->Pdxlop()->Edxloperatortype());
	
	if (fValidateChildren)
	{
		pdxlnJoinFilter->Pdxlop()->AssertValid(pdxlnJoinFilter, fValidateChildren);
		pdxlnLeft->Pdxlop()->AssertValid(pdxlnLeft, fValidateChildren);
		pdxlnRight->Pdxlop()->AssertValid(pdxlnRight, fValidateChildren);
	}
}
#endif // GPOS_DEBUG

// EOF
