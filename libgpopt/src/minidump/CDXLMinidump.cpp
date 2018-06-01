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
	DXLNodeArray *query_output_dxlnode_array,
	DXLNodeArray *cte_dxlnode_array,
	CDXLNode *pdxlnPlan, 
	DrgPimdobj *pdrgpmdobj,
	DrgPsysid *pdrgpsysid,
	ULLONG plan_id,
	ULLONG plan_space_size
	)
	:
	m_pbs(pbs),
	m_optimizer_config(optimizer_config),
	m_query_dxl_root(pdxlnQuery),
	m_pdrgpdxlnQueryOutput(query_output_dxlnode_array),
	m_cte_producer_dxl_array(cte_dxlnode_array),
	m_plan_dxl_root(pdxlnPlan),
	m_mdid_cached_obj_array(pdrgpmdobj),
	m_system_id_array(pdrgpsysid),
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
	CRefCount::SafeRelease(m_optimizer_config);
	CRefCount::SafeRelease(m_query_dxl_root);
	CRefCount::SafeRelease(m_pdrgpdxlnQueryOutput);
	CRefCount::SafeRelease(m_cte_producer_dxl_array);
	CRefCount::SafeRelease(m_plan_dxl_root);
	CRefCount::SafeRelease(m_mdid_cached_obj_array);
	CRefCount::SafeRelease(m_system_id_array);
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
//		CDXLMinidump::GetQueryDXLRoot
//
//	@doc:
//		Query object
//
//---------------------------------------------------------------------------
const CDXLNode *
CDXLMinidump::GetQueryDXLRoot() const
{
	return m_query_dxl_root;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::PdrgpdxlnQueryOutput
//
//	@doc:
//		Query output columns
//
//---------------------------------------------------------------------------
const DXLNodeArray *
CDXLMinidump::PdrgpdxlnQueryOutput() const
{
	return m_pdrgpdxlnQueryOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetCTEProducerDXLArray
//
//	@doc:
//		CTE list
//
//---------------------------------------------------------------------------
const DXLNodeArray *
CDXLMinidump::GetCTEProducerDXLArray() const
{
	return m_cte_producer_dxl_array;
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
	return m_plan_dxl_root;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetMdIdCachedObjArray
//
//	@doc:
//		Metadata objects
//
//---------------------------------------------------------------------------
const DrgPimdobj *
CDXLMinidump::GetMdIdCachedObjArray() const
{
	return m_mdid_cached_obj_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetSystemIdArray
//
//	@doc:
//		Metadata source system ids
//
//---------------------------------------------------------------------------
const DrgPsysid *
CDXLMinidump::GetSystemIdArray() const
{
	return m_system_id_array;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetPlanId
//
//	@doc:
//		Returns plan id
//
//---------------------------------------------------------------------------
ULLONG
CDXLMinidump::GetPlanId() const
{
	return m_plan_id;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLMinidump::GetPlanId
//
//	@doc:
//		Returns plan space size
//
//---------------------------------------------------------------------------
ULLONG
CDXLMinidump::GetPlanSpaceSize() const
{
	return m_plan_space_size;
}


// EOF
