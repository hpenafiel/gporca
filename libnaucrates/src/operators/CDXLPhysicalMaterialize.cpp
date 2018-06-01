//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalMaterialize.cpp
//
//	@doc:
//		Implementation of DXL physical materialize operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLPhysicalMaterialize.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::CDXLPhysicalMaterialize
//
//	@doc:
//		Construct a non-spooling materialize
//
//---------------------------------------------------------------------------
CDXLPhysicalMaterialize::CDXLPhysicalMaterialize
	(
	IMemoryPool *memory_pool,
	BOOL fEager
	)
	:
	CDXLPhysical(memory_pool),
	m_fEager(fEager),
	m_ulSpoolId(0),
	m_edxlsptype(EdxlspoolNone),
	m_iExecutorSlice(-1),
	m_ulConsumerSlices(0)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::CDXLPhysicalMaterialize
//
//	@doc:
//		Construct a spooling materialize
//
//---------------------------------------------------------------------------
CDXLPhysicalMaterialize::CDXLPhysicalMaterialize
	(
	IMemoryPool *memory_pool,
	BOOL fEager,
	ULONG ulSpoolId,
	INT iExecutorSlice,
	ULONG ulConsumerSlices
	)
	:
	CDXLPhysical(memory_pool),
	m_fEager(fEager),
	m_ulSpoolId(ulSpoolId),
	m_edxlsptype(EdxlspoolMaterialize),
	m_iExecutorSlice(iExecutorSlice),
	m_ulConsumerSlices(ulConsumerSlices)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalMaterialize::Edxlop() const
{
	return EdxlopPhysicalMaterialize;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalMaterialize::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalMaterialize);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::FSpooling
//
//	@doc:
//		Is this a spooling materialize operator
//
//---------------------------------------------------------------------------
BOOL
CDXLPhysicalMaterialize::FSpooling() const
{
	return (EdxlspoolNone != m_edxlsptype);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::UlSpoolId
//
//	@doc:
//		Id of the spool if the materialize node is spooling
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalMaterialize::UlSpoolId() const
{
	return m_ulSpoolId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::IExecutorSlice
//
//	@doc:
//		Id of the slice executing the spool
//
//---------------------------------------------------------------------------
INT
CDXLPhysicalMaterialize::IExecutorSlice() const
{
	return m_iExecutorSlice;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::UlConsumerSlices
//
//	@doc:
//		Number of slices consuming the spool
//
//---------------------------------------------------------------------------
ULONG
CDXLPhysicalMaterialize::UlConsumerSlices() const
{
	return m_ulConsumerSlices;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::FEager
//
//	@doc:
//		Does the materialize node do eager materialization
//
//---------------------------------------------------------------------------
BOOL
CDXLPhysicalMaterialize::FEager() const
{
	return m_fEager;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMaterialize::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenMaterializeEager), m_fEager);

	if (EdxlspoolMaterialize == m_edxlsptype)
	{
		// serialize spool info
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSpoolId), m_ulSpoolId);
		
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenExecutorSliceId), m_iExecutorSlice);
		xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenConsumerSliceCount), m_ulConsumerSlices);
	}
		
	// serialize properties
	pdxln->SerializePropertiesToDXL(xml_serializer);

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMaterialize::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMaterialize::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) 
	const
{
	GPOS_ASSERT(EdxlspoolNone == m_edxlsptype || EdxlspoolMaterialize == m_edxlsptype);
	GPOS_ASSERT(EdxlmatIndexSentinel == pdxln->Arity());

	CDXLNode *child_dxlnode = (*pdxln)[EdxlmatIndexChild];
	GPOS_ASSERT(EdxloptypePhysical == child_dxlnode->GetOperator()->Edxloperatortype());

	if (validate_children)
	{
		child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
