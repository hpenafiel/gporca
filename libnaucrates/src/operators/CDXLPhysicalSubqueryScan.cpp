//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalSubqueryScan.cpp
//
//	@doc:
//		Implementation of DXL physical subquery scan operators
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalSubqueryScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::CDXLPhysicalSubqueryScan
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSubqueryScan::CDXLPhysicalSubqueryScan
	(
	IMemoryPool *memory_pool,
	CMDName *mdname
	)
	:
	CDXLPhysical(memory_pool),
	m_pmdnameAlias(mdname)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::~CDXLPhysicalSubqueryScan
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLPhysicalSubqueryScan::~CDXLPhysicalSubqueryScan()
{
	GPOS_DELETE(m_pmdnameAlias);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalSubqueryScan::Edxlop() const
{
	return EdxlopPhysicalSubqueryScan;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalSubqueryScan::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalSubqueryScan);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::MdName
//
//	@doc:
//		Name for the subquery
//
//---------------------------------------------------------------------------
const CMDName *
CDXLPhysicalSubqueryScan::MdName()
{
	return m_pmdnameAlias;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSubqueryScan::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenAlias), m_pmdnameAlias->Pstr());
	
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);
	
	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);
		
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);		
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalSubqueryScan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysicalSubqueryScan::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);
	
	// subquery scan has 3 children
	GPOS_ASSERT(EdxlsubqscanIndexSentinel == pdxln->Arity());
	
	CDXLNode *child_dxlnode = (*pdxln)[EdxlsubqscanIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->Edxloperatortype());
	
	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
	
	// assert validity of table descriptor
	GPOS_ASSERT(NULL != m_pmdnameAlias);
	GPOS_ASSERT(m_pmdnameAlias->Pstr()->IsValid());
}
#endif // GPOS_DEBUG

// EOF
