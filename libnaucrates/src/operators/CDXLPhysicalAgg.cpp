//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalAgg.cpp
//
//	@doc:
//		Implementation of DXL physical aggregate operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalAgg.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::CDXLPhysicalAgg
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalAgg::CDXLPhysicalAgg
	(
	IMemoryPool *memory_pool,
	EdxlAggStrategy edxlaggstr,
	BOOL fStreamSafe
	)
	:
	CDXLPhysical(memory_pool),
	m_pdrgpulGroupingCols(NULL),
	m_edxlaggstr(edxlaggstr),
	m_fStreamSafe(fStreamSafe)
{
	GPOS_ASSERT_IMP(fStreamSafe, (EdxlaggstrategyHashed == edxlaggstr));
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::~CDXLPhysicalAgg
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalAgg::~CDXLPhysicalAgg()
{
	CRefCount::SafeRelease(m_pdrgpulGroupingCols);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalAgg::GetDXLOperator() const
{
	return EdxlopPhysicalAgg;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::Edxlaggstr
//
//	@doc:
//		Aggregation strategy
//
//---------------------------------------------------------------------------
EdxlAggStrategy
CDXLPhysicalAgg::Edxlaggstr() const
{
	return m_edxlaggstr;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalAgg::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalAggregate);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::PstrAggStrategy
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalAgg::PstrAggStrategy() const
{
	switch (m_edxlaggstr)
	{
		case EdxlaggstrategyPlain:
			return CDXLTokens::PstrToken(EdxltokenAggStrategyPlain);
		case EdxlaggstrategySorted:
			return CDXLTokens::PstrToken(EdxltokenAggStrategySorted);
		case EdxlaggstrategyHashed:
			return CDXLTokens::PstrToken(EdxltokenAggStrategyHashed);
		default:
			GPOS_ASSERT(!"Unrecognized aggregation strategy");
			return NULL;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::GetGroupingColidArray
//
//	@doc:
//		Grouping column indices
//
//---------------------------------------------------------------------------
const ULongPtrArray *
CDXLPhysicalAgg::GetGroupingColidArray() const
{
	return m_pdrgpulGroupingCols; 
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::SetGroupingCols
//
//	@doc:
//		Sets array of grouping columns
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAgg::SetGroupingCols(ULongPtrArray *pdrgpul)
{
	GPOS_ASSERT(NULL != pdrgpul);
	m_pdrgpulGroupingCols = pdrgpul;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::SerializeGroupingColsToDXL
//
//	@doc:
//		Serialize grouping column indices in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAgg::SerializeGroupingColsToDXL
	(
	CXMLSerializer *xml_serializer
	)
	const
{
	GPOS_ASSERT(NULL != m_pdrgpulGroupingCols);
	
	const CWStringConst *pstrTokenGroupingCols = CDXLTokens::PstrToken(EdxltokenGroupingCols);
	const CWStringConst *pstrTokenGroupingCol = CDXLTokens::PstrToken(EdxltokenGroupingCol);
		
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenGroupingCols);
	
	for (ULONG ul = 0; ul < m_pdrgpulGroupingCols->Size(); ul++)
	{
		GPOS_ASSERT(NULL != (*m_pdrgpulGroupingCols)[ul]);
		ULONG ulGroupingCol = *((*m_pdrgpulGroupingCols)[ul]);
		
		xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenGroupingCol);
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColId), ulGroupingCol);
		xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenGroupingCol);
	}
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), pstrTokenGroupingCols);	
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAgg::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAggStrategy), PstrAggStrategy());
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAggStreamSafe), m_fStreamSafe);
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	SerializeGroupingColsToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAgg::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAgg::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	GPOS_ASSERT((EdxlaggstrategySentinel > m_edxlaggstr) && (EdxlaggstrategyPlain <= m_edxlaggstr));

	GPOS_ASSERT(EdxlaggIndexSentinel == pdxln->Arity());
	GPOS_ASSERT(NULL != m_pdrgpulGroupingCols);
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlaggIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->GetDXLOperatorType());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
