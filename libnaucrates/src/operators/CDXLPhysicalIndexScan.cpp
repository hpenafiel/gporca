//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalIndexScan.cpp
//
//	@doc:
//		Implementation of DXL physical index scan operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalIndexScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::CDXLPhysicalIndexScan
//
//	@doc:
//		Construct an index scan node given its table descriptor,
//		index descriptor and filter conditions on the index
//
//---------------------------------------------------------------------------
CDXLPhysicalIndexScan::CDXLPhysicalIndexScan
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *pdxltabdesc,
	CDXLIndexDescr *pdxlid,
	EdxlIndexScanDirection edxlisd
	)
	:
	CDXLPhysical(memory_pool),
	m_pdxltabdesc(pdxltabdesc),
	m_pdxlid(pdxlid),
	m_edxlisd(edxlisd)
{
	GPOS_ASSERT(NULL != m_pdxltabdesc);
	GPOS_ASSERT(NULL != m_pdxlid);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::~CDXLPhysicalIndexScan
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalIndexScan::~CDXLPhysicalIndexScan()
{
	m_pdxlid->Release();
	m_pdxltabdesc->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalIndexScan::GetDXLOperator() const
{
	return EdxlopPhysicalIndexScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalIndexScan::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalIndexScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::Pdxlid
//
//	@doc:
//		Index descriptor for the index scan
//
//---------------------------------------------------------------------------
const CDXLIndexDescr *
CDXLPhysicalIndexScan::Pdxlid() const
{
	return m_pdxlid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::EdxlScanDirection
//
//	@doc:
//		Return the scan direction of the index
//
//---------------------------------------------------------------------------
EdxlIndexScanDirection
CDXLPhysicalIndexScan::EdxlScanDirection() const
{
	return m_edxlisd;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::Pdxltabdesc
//
//	@doc:
//		Return the associated table descriptor
//
//---------------------------------------------------------------------------
const CDXLTableDescr *
CDXLPhysicalIndexScan::Pdxltabdesc() const
{
	return m_pdxltabdesc;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalIndexScan::SerializeToDXL
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

	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	// serialize index descriptor
	m_pdxlid->SerializeToDXL(xml_serializer);

	// serialize table descriptor
	m_pdxltabdesc->SerializeToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexScan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalIndexScan::AssertValid
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
	GPOS_ASSERT(NULL != m_pdxlid);
	GPOS_ASSERT(NULL != m_pdxlid->MdName());
	GPOS_ASSERT(m_pdxlid->MdName()->Pstr()->IsValid());

	// assert validity of the table descriptor
	GPOS_ASSERT(NULL != m_pdxltabdesc);
	GPOS_ASSERT(NULL != m_pdxltabdesc->MdName());
	GPOS_ASSERT(m_pdxltabdesc->MdName()->Pstr()->IsValid());

	CDXLNode *pdxlnIndexConds = (*pdxln)[EdxlisIndexCondition];

	// assert children are of right type (physical/scalar)
	GPOS_ASSERT(EdxlopScalarIndexCondList == pdxlnIndexConds->GetOperator()->GetDXLOperator());

	if (validate_children)
	{
		pdxlnIndexConds->GetOperator()->AssertValid(pdxlnIndexConds, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
