//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLOperator.cpp
//
//	@doc:
//		Implementation of base DXL operator class
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLOperator.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperator::CDXLOperator
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLOperator::CDXLOperator
	(
	IMemoryPool *memory_pool
	)
	:
	m_memory_pool(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperator::~CDXLOperator
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLOperator::~CDXLOperator()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperator::GetJoinTypeNameStr
//
//	@doc:
//		Join type name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLOperator::GetJoinTypeNameStr
	(
	EdxlJoinType join_type
	)
{
	GPOS_ASSERT(EdxljtSentinel > join_type);

	switch (join_type)
	{
		case EdxljtInner:
			return CDXLTokens::PstrToken(EdxltokenJoinInner);

		case EdxljtLeft:
			return CDXLTokens::PstrToken(EdxltokenJoinLeft);

		case EdxljtFull:
			return CDXLTokens::PstrToken(EdxltokenJoinFull);

		case EdxljtRight:
			return CDXLTokens::PstrToken(EdxltokenJoinRight);

		case EdxljtIn:
			return CDXLTokens::PstrToken(EdxltokenJoinIn);

		case EdxljtLeftAntiSemijoin:
			return CDXLTokens::PstrToken(EdxltokenJoinLeftAntiSemiJoin);

		case EdxljtLeftAntiSemijoinNotIn:
			return CDXLTokens::PstrToken(EdxltokenJoinLeftAntiSemiJoinNotIn);

		default:
			return CDXLTokens::PstrToken(EdxltokenUnknown);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLOperator::GetIdxScanDirectionStr
//
//	@doc:
//		Return the index scan direction name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLOperator::GetIdxScanDirectionStr
	(
	EdxlIndexScanDirection idx_scan_direction
	)
{
	switch (idx_scan_direction)
	{
		case EdxlisdBackward:
			return CDXLTokens::PstrToken(EdxltokenIndexScanDirectionBackward);

		case EdxlisdForward:
			return CDXLTokens::PstrToken(EdxltokenIndexScanDirectionForward);

		case EdxlisdNoMovement:
			return CDXLTokens::PstrToken(EdxltokenIndexScanDirectionNoMovement);

		default:
			GPOS_ASSERT(!"Unrecognized index scan direction");
			return CDXLTokens::PstrToken(EdxltokenUnknown);
	}
}

// EOF
