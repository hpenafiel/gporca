//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalSetOp.cpp
//
//	@doc:
//		Implementation of DXL logical set operator
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/operators/CDXLLogicalSetOp.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"


using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::CDXLLogicalSetOp
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLLogicalSetOp::CDXLLogicalSetOp
	(
	IMemoryPool *memory_pool,
	EdxlSetOpType edxlsetoptype,
	ColumnDescrDXLArray *pdrgdxlcd,
	ULongPtrArray2D *ulong_ptr_array_2D,
	BOOL fCastAcrossInputs
	)
	:
	CDXLLogical(memory_pool),
	m_edxlsetoptype(edxlsetoptype),
	m_pdrgpdxlcd(pdrgdxlcd),
	m_pdrgpdrgpul(ulong_ptr_array_2D),
	m_fCastAcrossInputs(fCastAcrossInputs)
{
	GPOS_ASSERT(NULL != m_pdrgpdxlcd);
	GPOS_ASSERT(NULL != m_pdrgpdrgpul);
	GPOS_ASSERT(EdxlsetopSentinel > edxlsetoptype);
	
#ifdef GPOS_DEBUG	
	const ULONG ulCols = m_pdrgpdxlcd->Size();
	const ULONG ulLen = m_pdrgpdrgpul->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		ULongPtrArray *pdrgpulInput = (*m_pdrgpdrgpul)[ul];
		GPOS_ASSERT(ulCols == pdrgpulInput->Size());
	}

#endif	
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::~CDXLLogicalSetOp
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLLogicalSetOp::~CDXLLogicalSetOp()
{
	m_pdrgpdxlcd->Release();
	m_pdrgpdrgpul->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLLogicalSetOp::Edxlop() const
{
	return EdxlopLogicalSetOp;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLLogicalSetOp::PstrOpName() const
{
	switch (m_edxlsetoptype)
	{
		case EdxlsetopUnion:
				return CDXLTokens::PstrToken(EdxltokenLogicalUnion);

		case EdxlsetopUnionAll:
				return CDXLTokens::PstrToken(EdxltokenLogicalUnionAll);

		case EdxlsetopIntersect:
				return CDXLTokens::PstrToken(EdxltokenLogicalIntersect);

		case EdxlsetopIntersectAll:
				return CDXLTokens::PstrToken(EdxltokenLogicalIntersectAll);

		case EdxlsetopDifference:
				return CDXLTokens::PstrToken(EdxltokenLogicalDifference);

		case EdxlsetopDifferenceAll:
				return CDXLTokens::PstrToken(EdxltokenLogicalDifferenceAll);

		default:
			GPOS_ASSERT(!"Unrecognized set operator type");
			return NULL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLLogicalSetOp::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serialize the array of input colid arrays
	CWStringDynamic *pstrInputColIds = CDXLUtils::Serialize(m_memory_pool, m_pdrgpdrgpul);
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInputCols), pstrInputColIds);
	GPOS_DELETE(pstrInputColIds);
	
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenCastAcrossInputs), m_fCastAcrossInputs);

	// serialize output columns
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));
	GPOS_ASSERT(NULL != m_pdrgpdxlcd);

	const ULONG ulLen = m_pdrgpdxlcd->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		CDXLColDescr *pdxlcd = (*m_pdrgpdxlcd)[ul];
		pdxlcd->SerializeToDXL(xml_serializer);
	}
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenColumns));

	// serialize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::FDefinesColumn
//
//	@doc:
//		Check if given column is defined by operator
//
//---------------------------------------------------------------------------
BOOL
CDXLLogicalSetOp::FDefinesColumn
	(
	ULONG ulColId
	)
	const
{
	const ULONG ulSize = Arity();
	for (ULONG ulDescr = 0; ulDescr < ulSize; ulDescr++)
	{
		ULONG ulId = GetColumnDescrAt(ulDescr)->Id();
		if (ulId == ulColId)
		{
			return true;
		}
	}

	return false;
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLLogicalSetOp::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLLogicalSetOp::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(2 <= pdxln->Arity());
	GPOS_ASSERT(NULL != m_pdrgpdxlcd);

	// validate output columns
	const ULONG ulOutputCols = m_pdrgpdxlcd->Size();
	GPOS_ASSERT(0 < ulOutputCols);

	// validate children
	const ULONG ulChildren = pdxln->Arity();
	for (ULONG ul = 0; ul < ulChildren; ++ul)
	{
		CDXLNode *child_dxlnode = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeLogical == child_dxlnode->GetOperator()->Edxloperatortype());

		if (validate_children)
		{
			child_dxlnode->GetOperator()->AssertValid(child_dxlnode, validate_children);
		}
	}
}

#endif // GPOS_DEBUG

// EOF
