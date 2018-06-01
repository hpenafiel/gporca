//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDXLLogicalLimit.cpp
//
//	@doc:
//		Implementation of DXL logical limit operator
//		
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLLogicalLimit.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalLimit::CDXLLogicalLimit
//
//	@doc:
//		Construct a DXL Logical limit node
//
//---------------------------------------------------------------------------
CDXLLogicalLimit::CDXLLogicalLimit
	(
	IMemoryPool *memory_pool,
	BOOL fNonRemovableLimit
	)
	:
	CDXLLogical(memory_pool),
	m_fTopLimitUnderDML(fNonRemovableLimit)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalLimit::~CDXLLogicalLimit
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLLogicalLimit::~CDXLLogicalLimit()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalLimit::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalLimit::GetDXLOperator() const
{
	return EdxlopLogicalLimit;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalLimit::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalLimit::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenLogicalLimit);
}
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalLimit::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalLimit::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	if (m_fTopLimitUnderDML)
	{
		xml_serializer->AddAttribute
					(
					CDXLTokens::PstrToken(EdxltokenTopLimitUnderDML),
					m_fTopLimitUnderDML
					);
	}

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalLimit::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalLimit::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(4 == pdxln->Arity());

	// Assert the validity of sort column list
	CDXLNode *sort_col_list_dxl = (*pdxln)[EdxllogicallimitIndexSortColList];
	GPOS_ASSERT(EdxloptypeScalar == sort_col_list_dxl->GetOperator()->GetDXLOperatorType());

	// Assert the validity of Count and Offset

	CDXLNode *pdxlnCount = (*pdxln)[EdxllogicallimitIndexLimitCount];
	GPOS_ASSERT(EdxlopScalarLimitCount == pdxlnCount->GetOperator()->GetDXLOperator());
	
	CDXLNode *pdxlnOffset = (*pdxln)[EdxllogicallimitIndexLimitOffset];
	GPOS_ASSERT(EdxlopScalarLimitOffset == pdxlnOffset->GetOperator()->GetDXLOperator());
		
	// Assert child plan is a logical plan and is valid
	CDXLNode *child_dxlnode = (*pdxln)[EdxllogicallimitIndexChildPlan];
	GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		sort_col_list_dxl->GetOperator()->AssertValid(sort_col_list_dxl, validate_children);
		pdxlnOffset->GetOperator()->AssertValid(pdxlnOffset, validate_children);
		pdxlnCount->GetOperator()->AssertValid(pdxlnCount, validate_children);
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}

}
#endif // GPOS_DEBUG

// EOF
