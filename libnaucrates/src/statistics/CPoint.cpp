//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPoint.cpp
//
//	@doc:
//		Implementation of histogram point
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "naucrates/statistics/CPoint.h"
#include "gpopt/mdcache/CMDAccessor.h"

using namespace gpnaucrates;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CPoint::CPoint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPoint::CPoint
	(
	IDatum *pdatum
	)
	: 
	m_pdatum(pdatum)
{
	GPOS_ASSERT(NULL != m_pdatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::Equals
//
//	@doc:
//		Equality check
//
//---------------------------------------------------------------------------
BOOL
CPoint::Equals
	(
	const CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);
	return m_pdatum->FStatsEqual(ppoint->m_pdatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::FNotEqual
//
//	@doc:
//		Inequality check
//
//---------------------------------------------------------------------------
BOOL
CPoint::FNotEqual
	(
	const CPoint *ppoint
	)
	const
{
	return !(this->Equals(ppoint));
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::FLessThan
//
//	@doc:
//		Less than check
//
//---------------------------------------------------------------------------
BOOL
CPoint::FLessThan
	(
	const CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);
	return m_pdatum->FStatsComparable(ppoint->m_pdatum) && m_pdatum->FStatsLessThan(ppoint->m_pdatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::FLessThanOrEqual
//
//	@doc:
//		Less than or equals check
//
//---------------------------------------------------------------------------
BOOL
CPoint::FLessThanOrEqual
	(
	const CPoint *ppoint
	)
	const
{
	return (this->FLessThan(ppoint) || this->Equals(ppoint));
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::FGreaterThan
//
//	@doc:
//		Greater than check
//
//---------------------------------------------------------------------------
BOOL
CPoint::FGreaterThan
	(
	const CPoint *ppoint
	)
	const
{
	return m_pdatum->FStatsComparable(ppoint->m_pdatum) && m_pdatum->FStatsGreaterThan(ppoint->m_pdatum);
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::FGreaterThanOrEqual
//
//	@doc:
//		Greater than or equals check
//
//---------------------------------------------------------------------------
BOOL
CPoint::FGreaterThanOrEqual
	(
	const CPoint *ppoint
	)
	const
{
	return (this->FGreaterThan(ppoint) || this->Equals(ppoint));
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::DDistance
//
//	@doc:
//		DDistance between two points
//
//---------------------------------------------------------------------------
CDouble
CPoint::DDistance
	(
	const CPoint *ppoint
	)
	const
{
	GPOS_ASSERT(NULL != ppoint);
	if (m_pdatum->FStatsComparable(ppoint->m_pdatum))
	{
		return CDouble(m_pdatum->DStatsDistance(ppoint->m_pdatum));
	}

	// default to a non zero constant for overlap
	// computation
	return CDouble(1.0);
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream&
CPoint::OsPrint
	(
	IOstream &os
	)
	const
{
	m_pdatum->OsPrint(os);
	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::PpointMin
//
//	@doc:
//		minimum of two points using <=
//
//---------------------------------------------------------------------------
CPoint *
CPoint::PpointMin
	(
	CPoint *ppoint1,
	CPoint *ppoint2
	)
{
	if (ppoint1->FLessThanOrEqual(ppoint2))
	{
		return ppoint1;
	}
	return ppoint2;
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::PpointMax
//
//	@doc:
//		maximum of two points using >=
//
//---------------------------------------------------------------------------
CPoint *
CPoint::PpointMax
	(
	CPoint *ppoint1,
	CPoint *ppoint2
	)
{
	if (ppoint1->FGreaterThanOrEqual(ppoint2))
	{
		return ppoint1;
	}
	return ppoint2;
}

//---------------------------------------------------------------------------
//	@function:
//		CPoint::GetDatumVal
//
//	@doc:
//		Translate the point into its DXL representation
//---------------------------------------------------------------------------
CDXLDatum *
CPoint::GetDatumVal
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor
	)
	const
{
	IMDId *mdid = m_pdatum->MDId();
	return md_accessor->Pmdtype(mdid)->GetDatumVal(memory_pool, m_pdatum);
}

// EOF
