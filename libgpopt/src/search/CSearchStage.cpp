//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CSearchStage.cpp
//
//	@doc:
//		Implementation of optimizer search stage
//---------------------------------------------------------------------------

#include "gpopt/search/CSearchStage.h"
#include "gpopt/xforms/CXformFactory.h"

using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::CSearchStage
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CSearchStage::CSearchStage
	(
	CXformSet *pxfs,
	ULONG ulTimeThreshold,
	CCost costThreshold
	)
	:
	m_xforms(pxfs),
	m_time_threshold(ulTimeThreshold),
	m_cost_threshold(costThreshold),
	m_pexprBest(NULL),
	m_costBest(GPOPT_INVALID_COST)
{
	GPOS_ASSERT(NULL != pxfs);
	GPOS_ASSERT(0 < pxfs->Size());

	// include all implementation rules in any search strategy
	m_xforms->Union(CXformFactory::Pxff()->PxfsImplementation());
}


//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::~CSearchStage
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CSearchStage::~CSearchStage()
{
	m_xforms->Release();
	CRefCount::SafeRelease(m_pexprBest);
}


//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::OsPrint
//
//	@doc:
//		Print job
//
//---------------------------------------------------------------------------
IOstream &
CSearchStage::OsPrint
	(
	IOstream &os
	)
{
	os
		<< "Search Stage" << std::endl
		<< "\ttime threshold: " << m_time_threshold
		<< ", cost threshold:" << m_cost_threshold
		<< ", best plan found: " << std::endl;

	if (NULL != m_pexprBest)
	{
		os << *m_pexprBest;
	}

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::SetBestExpr
//
//	@doc:
//		Set best plan found at the end of search stage
//
//---------------------------------------------------------------------------
void
CSearchStage::SetBestExpr
	(
	CExpression *pexpr
	)
{
	GPOS_ASSERT_IMP(NULL != pexpr, pexpr->Pop()->FPhysical());

	m_pexprBest = pexpr;
	if (NULL != m_pexprBest)
	{
		m_costBest = m_pexprBest->Cost();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CSearchStage::PdrgpssDefault
//
//	@doc:
//		Generate default search strategy;
//		one stage with all xforms and no time/cost thresholds
//
//---------------------------------------------------------------------------
DrgPss *
CSearchStage::PdrgpssDefault
	(
	IMemoryPool *memory_pool
	)
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	pxfs->Union(CXformFactory::Pxff()->PxfsExploration());
	DrgPss *search_stage_array = GPOS_NEW(memory_pool) DrgPss(memory_pool);

	search_stage_array->Append(GPOS_NEW(memory_pool) CSearchStage(pxfs));

	return search_stage_array;
}

// EOF

