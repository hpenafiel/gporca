//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarSortCol.cpp
//
//	@doc:
//		Implementation of DXL sorting columns for sort and motion operator nodes
//---------------------------------------------------------------------------
#include "naucrates/dxl/operators/CDXLScalarSortCol.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/CDXLUtils.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::CDXLScalarSortCol
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSortCol::CDXLScalarSortCol
	(
	IMemoryPool *memory_pool,
	ULONG col_id,
	IMDId *pmdidSortOp,
	CWStringConst *pstrSortOpName,
	BOOL fSortNullsFirst
	)
	:
	CDXLScalar(memory_pool),
	m_ulColId(col_id),
	m_pmdidSortOp(pmdidSortOp),
	m_pstrSortOpName(pstrSortOpName),
	m_fSortNullsFirst(fSortNullsFirst)
{
	GPOS_ASSERT(m_pmdidSortOp->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::~CDXLScalarSortCol
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CDXLScalarSortCol::~CDXLScalarSortCol()
{
	m_pmdidSortOp->Release();
	GPOS_DELETE(m_pstrSortOpName);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSortCol::GetDXLOperator() const
{
	return EdxlopScalarSortCol;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSortCol::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSortCol);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::UlColId
//
//	@doc:
//		Id of the sorting column
//
//---------------------------------------------------------------------------
ULONG
CDXLScalarSortCol::UlColId() const
{
	return m_ulColId;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::PmdidSortOp
//
//	@doc:
//		Oid of the sorting operator for the column from the catalog
//
//---------------------------------------------------------------------------
IMDId *
CDXLScalarSortCol::PmdidSortOp() const
{
	return m_pmdidSortOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::FSortNullsFirst
//
//	@doc:
//		Whether nulls are sorted before other values
//
//---------------------------------------------------------------------------
BOOL
CDXLScalarSortCol::FSortNullsFirst() const
{
	return m_fSortNullsFirst;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarSortCol::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *// pdxln
	)
	const
{
	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenColId), m_ulColId);
	m_pmdidSortOp->Serialize(xml_serializer, CDXLTokens::PstrToken(EdxltokenSortOpId));	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSortOpName), m_pstrSortOpName);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenSortNullsFirst), m_fSortNullsFirst);
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSortCol::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLScalarSortCol::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL // validate_children
	) 
	const
{
	GPOS_ASSERT(0 == pdxln->Arity());
}
#endif // GPOS_DEBUG


// EOF
