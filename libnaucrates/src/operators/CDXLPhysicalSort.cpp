//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalSort.cpp
//
//	@doc:
//		Implementation of DXL physical sort operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalSort.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::CDXLPhysicalSort
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSort::CDXLPhysicalSort
	(
	IMemoryPool *memory_pool,
	BOOL fDiscardDuplicates
	)
	:
	CDXLPhysical(memory_pool),
	m_fDiscardDuplicates(fDiscardDuplicates)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalSort::GetDXLOperator() const
{
	return EdxlopPhysicalSort;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalSort::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalSort);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::FDiscardDuplicates
//
//	@doc:
//		Whether sort operator discards duplicated tuples.
//
//---------------------------------------------------------------------------
BOOL
CDXLPhysicalSort::FDiscardDuplicates() const
{
	return m_fDiscardDuplicates;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSort::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSortDiscardDuplicates), m_fDiscardDuplicates);
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSort::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT(EdxlsortIndexSentinel == pdxln->Arity());
	
	CDXLNode *sort_col_list_dxl = (*pdxln)[EdxlsortIndexSortColList];
	CDXLNode *child_dxlnode = (*pdxln)[EdxlsortIndexChild];
	CDXLNode *pdxlnLimitCount = (*pdxln)[EdxlsortIndexLimitCount];
	CDXLNode *pdxlnLimitOffset = (*pdxln)[EdxlsortIndexLimitOffset];
	
	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxloptypeScalar == sort_col_list_dxl->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	GPOS_ASSERT(EdxlopScalarLimitCount == pdxlnLimitCount->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxlopScalarLimitOffset == pdxlnLimitOffset->GetOperator()->GetDXLOperator());
	
	// there must be at least one sorting column
	GPOS_ASSERT(sort_col_list_dxl->Arity() > 0);
	
	if (validate_children)
	{
		sort_col_list_dxl->GetOperator()->AssertValid(sort_col_list_dxl, validate_children);
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
