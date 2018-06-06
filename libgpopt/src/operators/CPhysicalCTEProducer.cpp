//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalCTEProducer.cpp
//
//	@doc:
//		Implementation of CTE producer operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPhysicalCTEProducer.h"
#include "gpopt/operators/CPhysicalSpool.h"

#include "gpopt/base/CCTEMap.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::CPhysicalCTEProducer
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalCTEProducer::CPhysicalCTEProducer
	(
	IMemoryPool *memory_pool,
	ULONG id,
	DrgPcr *pdrgpcr
	)
	:
	CPhysical(memory_pool),
	m_id(id),
	m_pdrgpcr(pdrgpcr),
	m_pcrs(NULL)
{
	GPOS_ASSERT(NULL != pdrgpcr);
	m_pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, m_pdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::~CPhysicalCTEProducer
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalCTEProducer::~CPhysicalCTEProducer()
{
	m_pdrgpcr->Release();
	m_pcrs->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PcrsRequired
//
//	@doc:
//		Compute required output columns of the n-th child
//
//---------------------------------------------------------------------------
CColRefSet *
CPhysicalCTEProducer::PcrsRequired
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CColRefSet *pcrsRequired,
	ULONG ulChildIndex,
	DrgPdp *, // pdrgpdpCtxt
	ULONG // ulOptReq
	)
{
	GPOS_ASSERT(0 == ulChildIndex);
	GPOS_ASSERT(0 == pcrsRequired->Size());

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, *m_pcrs);
	pcrs->Union(pcrsRequired);
	CColRefSet *pcrsChildReqd = PcrsChildReqd(memory_pool, exprhdl, pcrs, ulChildIndex, ULONG_MAX);

	GPOS_ASSERT(pcrsChildReqd->Size() == m_pdrgpcr->Size());
	pcrs->Release();

	return pcrsChildReqd;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PosRequired
//
//	@doc:
//		Compute required sort order of the n-th child
//
//---------------------------------------------------------------------------
COrderSpec *
CPhysicalCTEProducer::PosRequired
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	COrderSpec *posRequired,
	ULONG ulChildIndex,
	DrgPdp *, // pdrgpdpCtxt
	ULONG // ulOptReq
	)
	const
{
	GPOS_ASSERT(0 == ulChildIndex);

	return PosPassThru(memory_pool, exprhdl, posRequired, ulChildIndex);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PdsRequired
//
//	@doc:
//		Compute required distribution of the n-th child
//
//---------------------------------------------------------------------------
CDistributionSpec *
CPhysicalCTEProducer::PdsRequired
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CDistributionSpec *pdsRequired,
	ULONG ulChildIndex,
	DrgPdp *, // pdrgpdpCtxt
	ULONG // ulOptReq
	)
	const
{
	GPOS_ASSERT(0 == ulChildIndex);

	return PdsPassThru(memory_pool, exprhdl, pdsRequired, ulChildIndex);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PrsRequired
//
//	@doc:
//		Compute required rewindability of the n-th child
//
//---------------------------------------------------------------------------
CRewindabilitySpec *
CPhysicalCTEProducer::PrsRequired
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CRewindabilitySpec *prsRequired,
	ULONG ulChildIndex,
	DrgPdp *, // pdrgpdpCtxt
	ULONG // ulOptReq
	)
	const
{
	GPOS_ASSERT(0 == ulChildIndex);

	return PrsPassThru(memory_pool, exprhdl, prsRequired, ulChildIndex);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PppsRequired
//
//	@doc:
//		Compute required partition propagation of the n-th child
//
//---------------------------------------------------------------------------
CPartitionPropagationSpec *
CPhysicalCTEProducer::PppsRequired
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CPartitionPropagationSpec *pppsRequired,
	ULONG ulChildIndex,
	DrgPdp *, //pdrgpdpCtxt,
	ULONG //ulOptReq
	)
{
	GPOS_ASSERT(0 == ulChildIndex);

	return PppsRequiredPushThru(memory_pool, exprhdl, pppsRequired, ulChildIndex);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PcteRequired
//
//	@doc:
//		Compute required CTE map of the n-th child
//
//---------------------------------------------------------------------------
CCTEReq *
CPhysicalCTEProducer::PcteRequired
	(
	IMemoryPool *, //memory_pool,
	CExpressionHandle &, //exprhdl,
	CCTEReq *pcter,
	ULONG
#ifdef GPOS_DEBUG
	ulChildIndex
#endif
	,
	DrgPdp *, //pdrgpdpCtxt,
	ULONG //ulOptReq
	)
	const
{
	GPOS_ASSERT(0 == ulChildIndex);
	return PcterPushThru(pcter);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PosDerive
//
//	@doc:
//		Derive sort order
//
//---------------------------------------------------------------------------
COrderSpec *
CPhysicalCTEProducer::PosDerive
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return PosDerivePassThruOuter(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PdsDerive
//
//	@doc:
//		Derive distribution
//
//---------------------------------------------------------------------------
CDistributionSpec *
CPhysicalCTEProducer::PdsDerive
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return PdsDerivePassThruOuter(exprhdl);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PrsDerive
//
//	@doc:
//		Derive rewindability
//
//---------------------------------------------------------------------------
CRewindabilitySpec *
CPhysicalCTEProducer::PrsDerive
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return PrsDerivePassThruOuter(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::PcmDerive
//
//	@doc:
//		Derive cte map
//
//---------------------------------------------------------------------------
CCTEMap *
CPhysicalCTEProducer::PcmDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	GPOS_ASSERT(1 == exprhdl.Arity());

	CCTEMap *pcmChild = exprhdl.Pdpplan(0)->GetCostModel();

	CCTEMap *pcmProducer = GPOS_NEW(memory_pool) CCTEMap(memory_pool);
	// store plan properties of the child in producer's CTE map
	pcmProducer->Insert(m_id, CCTEMap::EctProducer, exprhdl.Pdpplan(0));

	CCTEMap *pcmCombined = CCTEMap::PcmCombine(memory_pool, *pcmProducer, *pcmChild);
	pcmProducer->Release();

	return pcmCombined;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::FProvidesReqdCols
//
//	@doc:
//		Check if required columns are included in output columns
//
//---------------------------------------------------------------------------
BOOL
CPhysicalCTEProducer::FProvidesReqdCols
	(
	CExpressionHandle &exprhdl,
	CColRefSet *pcrsRequired,
	ULONG // ulOptReq
	)
	const
{
	return FUnaryProvidesReqdCols(exprhdl, pcrsRequired);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalCTEProducer::EpetOrder
	(
	CExpressionHandle &exprhdl,
	const CEnfdOrder *peo
	)
	const
{
	GPOS_ASSERT(NULL != peo);
	GPOS_ASSERT(!peo->PosRequired()->IsEmpty());

	COrderSpec *pos = CDrvdPropPlan::Pdpplan(exprhdl.Pdp())->Pos();
	if (peo->FCompatible(pos))
	{
		return CEnfdProp::EpetUnnecessary;
	}

	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::EpetRewindability
//
//	@doc:
//		Return the enforcing type for rewindability property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalCTEProducer::EpetRewindability
	(
	CExpressionHandle &exprhdl,
	const CEnfdRewindability *per
	)
	const
{
	GPOS_ASSERT(NULL != per);

	CRewindabilitySpec *prs = CDrvdPropPlan::Pdpplan(exprhdl.Pdp())->Prs();
	if (per->FCompatible(prs))
	{
		 return CEnfdProp::EpetUnnecessary;
	}

	return CEnfdProp::EpetRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::FMatch
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CPhysicalCTEProducer::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CPhysicalCTEProducer *popCTEProducer = CPhysicalCTEProducer::PopConvert(pop);

	return m_id == popCTEProducer->UlCTEId() &&
			m_pdrgpcr->Equals(popCTEProducer->Pdrgpcr());
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CPhysicalCTEProducer::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), m_id);
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcr));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalCTEProducer::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalCTEProducer::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " (";
	os << m_id;
	os << "), Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcr);
	os	<< "]";

	return os;
}

// EOF
