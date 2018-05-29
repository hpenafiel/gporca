//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDXLMinidump.cpp
//
//	@doc:
//		Implementation of DXL-based minidump object
//---------------------------------------------------------------------------

#include "gpos/common/CBitSet.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "gpopt/minidump/CDXLMinidump.h"
#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/engine/CStatisticsConfig.h"
#include "gpopt/optimizer/COptimizerConfig.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::CDXLMinidump
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLMinidump::CDXLMinidump
	(
	CBitSet *pbs,
	COptimizerConfig *optimizer_config,
	CDXLNode *pdxlnQuery, 
	DrgPdxln *query_output_dxlnode_array,
	DrgPdxln *cte_dxlnode_array,
	CDXLNode *pdxlnPlan, 
	DrgPimdobj *pdrgpmdobj,
	DrgPsysid *pdrgpsysid,
	ULLONG plan_id,
	ULLONG plan_space_size
	)
	:
	m_pbs(pbs),
	m_poconf(optimizer_config),
	m_pdxlnQuery(pdxlnQuery),
	m_pdrgpdxlnQueryOutput(query_output_dxlnode_array),
	m_pdrgpdxlnCTE(cte_dxlnode_array),
	m_pdxlnPlan(pdxlnPlan),
	m_pdrgpmdobj(pdrgpmdobj),
	m_pdrgpsysid(pdrgpsysid),
	m_plan_id(plan_id),
	m_plan_space_size(plan_space_size)
{}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::~CDXLMinidump
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLMinidump::~CDXLMinidump()
{
	// some of the structures may be NULL as they are not included in the minidump
	CRefCount::SafeRelease(m_pbs);
	CRefCount::SafeRelease(m_poconf);
	CRefCount::SafeRelease(m_pdxlnQuery);
	CRefCount::SafeRelease(m_pdrgpdxlnQueryOutput);
	CRefCount::SafeRelease(m_pdrgpdxlnCTE);
	CRefCount::SafeRelease(m_pdxlnPlan);
	CRefCount::SafeRelease(m_pdrgpmdobj);
	CRefCount::SafeRelease(m_pdrgpsysid);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::Pbs
//
//	@doc:
//		Traceflags
//
//---------------------------------------------------------------------------
const CBitSet *
CDXLMinidump::Pbs() const
{
	return m_pbs;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdxlnQuery
//
//	@doc:
//		Query object
//
//---------------------------------------------------------------------------
const CDXLNode *
CDXLMinidump::PdxlnQuery() const
{
	return m_pdxlnQuery;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdrgpdxlnQueryOutput
//
//	@doc:
//		Query output columns
//
//---------------------------------------------------------------------------
const DrgPdxln *
CDXLMinidump::PdrgpdxlnQueryOutput() const
{
	return m_pdrgpdxlnQueryOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdrgpdxlnCTE
//
//	@doc:
//		CTE list
//
//---------------------------------------------------------------------------
const DrgPdxln *
CDXLMinidump::PdrgpdxlnCTE() const
{
	return m_pdrgpdxlnCTE;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdxlnPlan
//
//	@doc:
//		Query object
//
//---------------------------------------------------------------------------
const CDXLNode *
CDXLMinidump::PdxlnPlan() const
{
	return m_pdxlnPlan;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::Pdrgpmdobj
//
//	@doc:
//		Metadata objects
//
//---------------------------------------------------------------------------
const DrgPimdobj *
CDXLMinidump::Pdrgpmdobj() const
{
	return m_pdrgpmdobj;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::Pdrgpsysid
//
//	@doc:
//		Metadata source system ids
//
//---------------------------------------------------------------------------
const DrgPsysid *
CDXLMinidump::Pdrgpsysid() const
{
	return m_pdrgpsysid;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::UllPlanId
//
//	@doc:
//		Returns plan id
//
//---------------------------------------------------------------------------
ULLONG
CDXLMinidump::UllPlanId() const
{
	return m_plan_id;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::UllPlanId
//
//	@doc:
//		Returns plan space size
//
//---------------------------------------------------------------------------
ULLONG
CDXLMinidump::UllPlanSpaceSize() const
{
	return m_plan_space_size;
}


// EOF
