//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLPhysicalPartitionSelector.cpp
//
//	@doc:
//		Implementation of DXL physical partition selector
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalPartitionSelector.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalPartitionSelector::CDXLPhysicalPartitionSelector
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalPartitionSelector::CDXLPhysicalPartitionSelector
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidRel,
	ULONG ulLevels,
	ULONG ulScanId
	)
	:
	CDXLPhysical(memory_pool),
	m_pmdidRel(pmdidRel),
	m_ulLevels(ulLevels),
	m_ulScanId(ulScanId)
{
	GPOS_ASSERT(pmdidRel->IsValid());
	GPOS_ASSERT(0 < ulLevels);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalPartitionSelector::~CDXLPhysicalPartitionSelector
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalPartitionSelector::~CDXLPhysicalPartitionSelector()
{
	m_pmdidRel->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalPartitionSelector::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalPartitionSelector::GetDXLOperator() const
{
	return EdxlopPhysicalPartitionSelector;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalPartitionSelector::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalPartitionSelector::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalPartitionSelector);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalPartitionSelector::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalPartitionSelector::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	m_pmdidRel->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenRelationMdid));
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPhysicalPartitionSelectorLevels), m_ulLevels);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPhysicalPartitionSelectorScanId), m_ulScanId);
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize project list and filter lists
	(*pdxln)[EdxlpsIndexProjList]->SerializeToDXL(xml_serializer);
	(*pdxln)[EdxlpsIndexEqFilters]->SerializeToDXL(xml_serializer);
	(*pdxln)[EdxlpsIndexFilters]->SerializeToDXL(xml_serializer);

	// serialize residual filter
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarResidualFilter));
	(*pdxln)[EdxlpsIndexResidualFilter]->SerializeToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarResidualFilter));

	// serialize propagation expression
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarPropagationExpr));
	(*pdxln)[EdxlpsIndexPropExpr]->SerializeToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarPropagationExpr));

	// serialize printable filter
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarPrintableFilter));
	(*pdxln)[EdxlpsIndexPrintableFilter]->SerializeToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarPrintableFilter));

	// serialize relational child, if any
	if (7 == pdxln->Arity())
	{
		(*pdxln)[EdxlpsIndexChild]->SerializeToDXL(xml_serializer);
	}

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalPartitionSelector::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalPartitionSelector::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(6 == ulArity || 7 == ulArity);
	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
