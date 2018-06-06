//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDynamicTableScan.cpp
//
//	@doc:
//		Implementation of DXL physical dynamic table scan operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalDynamicTableScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::CDXLPhysicalDynamicTableScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysicalDynamicTableScan::CDXLPhysicalDynamicTableScan
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *table_descr,
	ULONG part_idx_id,
	ULONG part_idx_id_printable
	)
	:
	CDXLPhysical(memory_pool),
	m_table_descr_dxl(table_descr),
	m_part_index_id(part_idx_id),
	m_part_index_id_printable(part_idx_id_printable)
{
	GPOS_ASSERT(NULL != table_descr);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::~CDXLPhysicalDynamicTableScan
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalDynamicTableScan::~CDXLPhysicalDynamicTableScan()
{
	m_table_descr_dxl->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalDynamicTableScan::GetDXLOperator() const
{
	return EdxlopPhysicalDynamicTableScan;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalDynamicTableScan::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalDynamicTableScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::GetTableDescr
//
//	@doc:
//		Table descriptor for the table scan
//
//---------------------------------------------------------------------------
const CDXLTableDescr *
CDXLPhysicalDynamicTableScan::GetTableDescr() const
{
	return m_table_descr_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::GetPartIndexId
//
//	@doc:
//		Id of partition index
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalDynamicTableScan::GetPartIndexId() const
{
	return m_part_index_id;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::GetPartIndexIdPrintable
//
//	@doc:
//		Printable partition index id
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalDynamicTableScan::GetPartIndexIdPrintable() const
{
	return m_part_index_id_printable;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalDynamicTableScan::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *node
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartIndexId), m_part_index_id);
	if (m_part_index_id_printable != m_part_index_id)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartIndexIdPrintable), m_part_index_id_printable);
	}
	node->SerializePropertiesToDXL(xml_serializer);
	node->SerializeChildrenToDXL(xml_serializer);
	m_table_descr_dxl->SerializeToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalDynamicTableScan::AssertValid
	(
	const CDXLNode *node,
	BOOL // validate_children
	) 
	const
{
	GPOS_ASSERT(2 == node->Arity());
	
	// assert validity of table descriptor
	GPOS_ASSERT(NULL != m_table_descr_dxl);
	GPOS_ASSERT(NULL != m_table_descr_dxl->MdName());
	GPOS_ASSERT(m_table_descr_dxl->MdName()->GetMDName()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
