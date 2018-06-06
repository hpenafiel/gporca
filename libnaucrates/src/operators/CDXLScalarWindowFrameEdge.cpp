//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarWindowFrameEdge.cpp
//
//	@doc:
//		Implementation of DXL scalar window frame edge
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLScalarWindowFrameEdge.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowFrameEdge::CDXLScalarWindowFrameEdge
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLScalarWindowFrameEdge::CDXLScalarWindowFrameEdge
	(
	IMemoryPool *memory_pool,
	BOOL fLeading,
	EdxlFrameBoundary edxlfb
	)
	:
	CDXLScalar(memory_pool),
	m_fLeading(fLeading),
	m_edxlfb(edxlfb)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowFrameEdge::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarWindowFrameEdge::GetDXLOperator() const
{
	return EdxlopScalarWindowFrameEdge;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowFrameEdge::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarWindowFrameEdge::GetOpNameStr() const
{
	if (m_fLeading)
	{
		return CDXLTokens::GetDXLTokenStr(EdxltokenScalarWindowFrameLeadingEdge);
	}

	return CDXLTokens::GetDXLTokenStr(EdxltokenScalarWindowFrameTrailingEdge);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowFrameEdge::PstrFrameBoundary
//
//	@doc:
//		Return the string representation of the window frame boundary
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarWindowFrameEdge::PstrFrameBoundary
	(
	EdxlFrameBoundary edxlfb
	)
	const
{
	GPOS_ASSERT(EdxlfbSentinel > edxlfb);

	ULONG rgrgulMapping[][2] =
					{
					{EdxlfbUnboundedPreceding, EdxltokenWindowBoundaryUnboundedPreceding},
					{EdxlfbBoundedPreceding, EdxltokenWindowBoundaryBoundedPreceding},
					{EdxlfbCurrentRow, EdxltokenWindowBoundaryCurrentRow},
					{EdxlfbUnboundedFollowing, EdxltokenWindowBoundaryUnboundedFollowing},
					{EdxlfbBoundedFollowing, EdxltokenWindowBoundaryBoundedFollowing},
					{EdxlfbDelayedBoundedPreceding, EdxltokenWindowBoundaryDelayedBoundedPreceding},
					{EdxlfbDelayedBoundedFollowing, EdxltokenWindowBoundaryDelayedBoundedFollowing}
					};

	const ULONG ulArity = GPOS_ARRAY_SIZE(rgrgulMapping);
	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		ULONG *pulElem = rgrgulMapping[ul];
		if ((ULONG) edxlfb == pulElem[0])
		{
			Edxltoken edxltk = (Edxltoken) pulElem[1];
			return CDXLTokens::GetDXLTokenStr(edxltk);
		}
	}

	GPOS_ASSERT(!"Unrecognized window frame boundary");
	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowFrameEdge::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarWindowFrameEdge::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode *pdxln
	)
	const
{

	const CWStringConst *element_name = GetOpNameStr();
	xml_serializer->OpenElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);

	if (m_fLeading)
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowLeadingBoundary), PstrFrameBoundary(m_edxlfb));
	}
	else
	{
		xml_serializer->AddAttribute(CDXLTokens::GetDXLTokenStr(EdxltokenWindowTrailingBoundary), PstrFrameBoundary(m_edxlfb));
	}

	pdxln->SerializeChildrenToDXL(xml_serializer);
	xml_serializer->CloseElement(CDXLTokens::GetDXLTokenStr(EdxltokenNamespacePrefix), element_name);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarWindowFrameEdge::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLScalarWindowFrameEdge::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	const ULONG ulArity = pdxln->Arity();
	GPOS_ASSERT(1 >= ulArity);

	GPOS_ASSERT_IMP((m_edxlfb == EdxlfbBoundedPreceding || m_edxlfb == EdxlfbBoundedFollowing
					|| m_edxlfb == EdxlfbDelayedBoundedPreceding || m_edxlfb == EdxlfbDelayedBoundedFollowing), 1 == ulArity);
	GPOS_ASSERT_IMP((m_edxlfb == EdxlfbUnboundedPreceding || m_edxlfb == EdxlfbUnboundedFollowing || m_edxlfb == EdxlfbCurrentRow), 0 == ulArity);

	for (ULONG ul = 0; ul < ulArity; ++ul)
	{
		CDXLNode *dxlnode_arg = (*pdxln)[ul];
		GPOS_ASSERT(EdxloptypeScalar == dxlnode_arg->GetOperator()->GetDXLOperatorType());

		if (validate_children)
		{
			dxlnode_arg->GetOperator()->AssertValid(dxlnode_arg, validate_children);
		}
	}
}
#endif // GPOS_DEBUG

// EOF
