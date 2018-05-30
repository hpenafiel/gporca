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
//		CDXLPhysicalSort::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalSort::Edxlop() const
{
	return EdxlopPhysicalSort;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSort::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalSort::PstrOpName() const
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
	const CWStringConst *pstrElemName = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);		
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSortDiscardDuplicates), m_fDiscardDuplicates);
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrElemName);
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
	BOOL fValidateChildren
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, fValidateChildren);
	
	GPOS_ASSERT(EdxlsortIndexSentinel == pdxln->UlArity());
	
	CDXLNode *pdxlnSortColList = (*pdxln)[EdxlsortIndexSortColList];
	CDXLNode *pdxlnChild = (*pdxln)[EdxlsortIndexChild];
	CDXLNode *pdxlnLimitCount = (*pdxln)[EdxlsortIndexLimitCount];
	CDXLNode *pdxlnLimitOffset = (*pdxln)[EdxlsortIndexLimitOffset];
	
	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxloptypeScalar == pdxlnSortColList->Pdxlop()->Edxloperatortype());
	GPOS_ASSERT(EdxloptypePhysical == pdxlnChild->Pdxlop()->Edxloperatortype());
	GPOS_ASSERT(EdxlopScalarLimitCount == pdxlnLimitCount->Pdxlop()->Edxlop());
	GPOS_ASSERT(EdxlopScalarLimitOffset == pdxlnLimitOffset->Pdxlop()->Edxlop());
	
	// there must be at least one sorting column
	GPOS_ASSERT(pdxlnSortColList->UlArity() > 0);
	
	if (fValidateChildren)
	{
		pdxlnSortColList->Pdxlop()->AssertValid(pdxlnSortColList, fValidateChildren);
		pdxlnChild->Pdxlop()->AssertValid(pdxlnChild, fValidateChildren);
	}
}
#endif // GPOS_DEBUG

// EOF
