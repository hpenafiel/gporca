//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalTableScan.cpp
//
//	@doc:
//		Implementation of DXL physical table scan operators
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalTableScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::CDXLPhysicalTableScan
//
//	@doc:
//		Construct a table scan node with uninitialized table descriptor
//
//---------------------------------------------------------------------------
CDXLPhysicalTableScan::CDXLPhysicalTableScan
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool),
	m_pdxltabdesc(NULL)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::CDXLPhysicalTableScan
//
//	@doc:
//		Construct a table scan node given its table descriptor
//
//---------------------------------------------------------------------------
CDXLPhysicalTableScan::CDXLPhysicalTableScan
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc
	)
	:CDXLPhysical(memory_pool),
	 m_pdxltabdesc(pdxltabdesc)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::~CDXLPhysicalTableScan
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalTableScan::~CDXLPhysicalTableScan()
{
	CRefCount::SafeRelease(m_pdxltabdesc);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::SetTableDescriptor
//
//	@doc:
//		Set table descriptor
//
//---------------------------------------------------------------------------
void
CDXLPhysicalTableScan::SetTableDescriptor
	(
	CDXLTableDescr *pdxltabdesc
	)
{
	// allow setting table descriptor only once
	GPOS_ASSERT (NULL == m_pdxltabdesc);
	
	m_pdxltabdesc = pdxltabdesc;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalTableScan::Edxlop() const
{
	return EdxlopPhysicalTableScan;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalTableScan::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalTableScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::Pdxltabdesc
//
//	@doc:
//		Table descriptor for the table scan
//
//---------------------------------------------------------------------------
const CDXLTableDescr *
CDXLPhysicalTableScan::Pdxltabdesc()
{
	return m_pdxltabdesc;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalTableScan::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
	
	// serialize table descriptor
	m_pdxltabdesc->SerializeToDXL(xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalTableScan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalTableScan::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	// table scan has only 2 children
	GPOS_ASSERT(2 == pdxln->Arity());
	
	// assert validity of table descriptor
	GPOS_ASSERT(NULL != m_pdxltabdesc);
	GPOS_ASSERT(NULL != m_pdxltabdesc->MdName());
	GPOS_ASSERT(m_pdxltabdesc->MdName()->Pstr()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
