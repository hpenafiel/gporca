//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CStatsPredLike.cpp
//
//	@doc:
//		Implementation of statistics LIKE filter
//---------------------------------------------------------------------------

#include "naucrates/statistics/CStatsPredLike.h"
#include "gpopt/operators/CExpression.h"

using namespace gpnaucrates;
using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::CStatisticsFilterLike
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CStatsPredLike::CStatsPredLike
	(
	ULONG col_id,
	CExpression *pexprLeft,
	CExpression *pexprRight,
	CDouble dDefaultScaleFactor
	)
	:
	CStatsPred(col_id),
	m_pexprLeft(pexprLeft),
	m_pexprRight(pexprRight),
	m_dDefaultScaleFactor(dDefaultScaleFactor)
{
	GPOS_ASSERT(ULONG_MAX != col_id);
	GPOS_ASSERT(NULL != pexprLeft);
	GPOS_ASSERT(NULL != pexprRight);
	GPOS_ASSERT(0 < dDefaultScaleFactor);
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::~CStatisticsFilterLike
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CStatsPredLike::~CStatsPredLike()
{
	m_pexprLeft->Release();
	m_pexprRight->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::GetColId
//
//	@doc:
//		Return the column identifier on which the predicates are on
//
//---------------------------------------------------------------------------
ULONG
CStatsPredLike::GetColId() const
{
	return m_colid;
}

//---------------------------------------------------------------------------
//	@function:
//		CStatsPredLike::DDefaultScaleFactor
//
//	@doc:
//		Return the default like scale factor
//
//---------------------------------------------------------------------------
CDouble
CStatsPredLike::DDefaultScaleFactor() const
{
	return m_dDefaultScaleFactor;
}

// EOF
