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
	ULONG ulPartIndexId,
	ULONG ulPartIndexIdPrintable
	)
	:
	CDXLPhysical(memory_pool),
	m_table_descr_dxl(table_descr),
	m_ulPartIndexId(ulPartIndexId),
	m_ulPartIndexIdPrintable(ulPartIndexIdPrintable)
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
//		CDXLPhysicalDynamicTableScan::UlPartIndexId
//
//	@doc:
//		Id of partition index
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalDynamicTableScan::UlPartIndexId() const
{
	return m_ulPartIndexId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalDynamicTableScan::UlPartIndexIdPrintable
//
//	@doc:
//		Printable partition index id
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalDynamicTableScan::UlPartIndexIdPrintable() const
{
	return m_ulPartIndexIdPrintable;
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
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartIndexId), m_ulPartIndexId);
	if (m_ulPartIndexIdPrintable != m_ulPartIndexId)
	{
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenPartIndexIdPrintable), m_ulPartIndexIdPrintable);
	}
	pdxln->SerializePropertiesToDXL(xml_serializer);
	pdxln->SerializeChildrenToDXL(xml_serializer);
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
	const CDXLNode *pdxln,
	BOOL // validate_children
	) 
	const
{
	GPOS_ASSERT(2 == pdxln->Arity());
	
	// assert validity of table descriptor
	GPOS_ASSERT(NULL != m_table_descr_dxl);
	GPOS_ASSERT(NULL != m_table_descr_dxl->MdName());
	GPOS_ASSERT(m_table_descr_dxl->MdName()->Pstr()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
