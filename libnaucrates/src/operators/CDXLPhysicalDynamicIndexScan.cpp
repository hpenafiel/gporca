//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDynamicIndexScan.cpp
//
//	@doc:
//		Implementation of DXL physical dynamic index scan operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalDynamicIndexScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::CDXLPhysicalDynamicIndexScan
//
//	@doc:
//		Construct an index scan node given its table descriptor,
//		index descriptor and filter conditions on the index
//
//---------------------------------------------------------------------------
CDXLPhysicalDynamicIndexScan::CDXLPhysicalDynamicIndexScan
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *table_descr,
	ULONG ulPartIndexId,
	ULONG ulPartIndexIdPrintable,
	CDXLIndexDescr *pdxlid,
	EdxlIndexScanDirection edxlisd
	)
	:
	CDXLPhysical(memory_pool),
	m_table_descr_dxl(table_descr),
	m_ulPartIndexId(ulPartIndexId),
	m_ulPartIndexIdPrintable(ulPartIndexIdPrintable),
	m_index_descr_dxl(pdxlid),
	m_edxlisd(edxlisd)
{
	GPOS_ASSERT(NULL != m_table_descr_dxl);
	GPOS_ASSERT(NULL != m_index_descr_dxl);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::~CDXLPhysicalDynamicIndexScan
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalDynamicIndexScan::~CDXLPhysicalDynamicIndexScan()
{
	m_index_descr_dxl->Release();
	m_table_descr_dxl->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalDynamicIndexScan::GetDXLOperator() const
{
	return EdxlopPhysicalDynamicIndexScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalDynamicIndexScan::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalDynamicIndexScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::GetIndexDescr
//
//	@doc:
//		Index descriptor for the index scan
//
//---------------------------------------------------------------------------
const CDXLIndexDescr *
CDXLPhysicalDynamicIndexScan::GetIndexDescr() const
{
	return m_index_descr_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::EdxlScanDirection
//
//	@doc:
//		Return the scan direction of the index
//
//---------------------------------------------------------------------------
EdxlIndexScanDirection
CDXLPhysicalDynamicIndexScan::EdxlScanDirection() const
{
	return m_edxlisd;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::GetTableDescr
//
//	@doc:
//		Return the associated table descriptor
//
//---------------------------------------------------------------------------
const CDXLTableDescr *
CDXLPhysicalDynamicIndexScan::GetTableDescr() const
{
	return m_table_descr_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::UlPartIndexId
//
//	@doc:
//		Part index id
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalDynamicIndexScan::UlPartIndexId() const
{
	return m_ulPartIndexId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::UlPartIndexIdPrintable
//
//	@doc:
//		Printable partition index id
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalDynamicIndexScan::UlPartIndexIdPrintable() const
{
	return m_ulPartIndexIdPrintable;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalDynamicIndexScan::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute
				(
				CDXLTokens::PstrToken(EdxltokenIndexScanDirection),
				CDXLOperator::PstrIndexScanDirection(m_edxlisd)
				);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartIndexId), m_ulPartIndexId);
	if (m_ulPartIndexIdPrintable != m_ulPartIndexId)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartIndexIdPrintable), m_ulPartIndexIdPrintable);
	}

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	// serialize index descriptor
	m_index_descr_dxl->SerializeToDXL(xml_serializer);

	// serialize table descriptor
	m_table_descr_dxl->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicIndexScan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalDynamicIndexScan::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);

	// index scan has only 3 children
	GPOS_ASSERT(3 == pdxln->Arity());

	// assert validity of the index descriptor
	GPOS_ASSERT(NULL != m_index_descr_dxl);
	GPOS_ASSERT(NULL != m_index_descr_dxl->MdName());
	GPOS_ASSERT(m_index_descr_dxl->MdName()->Pstr()->IsValid());

	// assert validity of the table descriptor
	GPOS_ASSERT(NULL != m_table_descr_dxl);
	GPOS_ASSERT(NULL != m_table_descr_dxl->MdName());
	GPOS_ASSERT(m_table_descr_dxl->MdName()->Pstr()->IsValid());

	CDXLNode *pdxlnIndexFilter = (*pdxln)[EdxldisIndexFilter];
	CDXLNode *pdxlnIndexConds = (*pdxln)[EdxldisIndexCondition];

	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxlopScalarIndexCondList == pdxlnIndexConds->GetOperator()->GetDXLOperator());
	GPOS_ASSERT(EdxlopScalarFilter == pdxlnIndexFilter->GetOperator()->GetDXLOperator());

	if (validate_children)
	{
		pdxlnIndexConds->GetOperator()->AssertValid(pdxlnIndexConds, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
