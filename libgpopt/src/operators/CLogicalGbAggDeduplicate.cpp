//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 Pivotal, Inc.
//
//	@filename:
//		CLogicalGbAggDeduplicate.cpp
//
//	@doc:
//		Implementation of aggregate operator for deduplicating semijoin outputs
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CKeyCollection.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGbAggDeduplicate.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
//
//	@doc:
//		Ctor for xform pattern
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalGbAgg(memory_pool),
	m_pdrgpcrKeys(NULL)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr,
	COperator::EGbAggType egbaggtype,
	DrgPcr *pdrgpcrKeys
	)
	:
	CLogicalGbAgg(memory_pool, pdrgpcr, egbaggtype),
	m_pdrgpcrKeys(pdrgpcrKeys)
{
	GPOS_ASSERT(NULL != pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::CLogicalGbAggDeduplicate
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr,
	DrgPcr *pdrgpcrMinimal,
	COperator::EGbAggType egbaggtype,
	DrgPcr *pdrgpcrKeys
	)
	:
	CLogicalGbAgg(memory_pool, pdrgpcr, pdrgpcrMinimal, egbaggtype),
	m_pdrgpcrKeys(pdrgpcrKeys)
{
	GPOS_ASSERT(NULL != pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::~CLogicalGbAggDeduplicate
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalGbAggDeduplicate::~CLogicalGbAggDeduplicate()
{
	// safe release -- to allow for instances used in patterns
	CRefCount::SafeRelease(m_pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalGbAggDeduplicate::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcr = CUtils::PdrgpcrRemap(memory_pool, Pdrgpcr(), phmulcr, fMustExist);

	DrgPcr *pdrgpcrMinimal = PdrgpcrMinimal();
	if (NULL != pdrgpcrMinimal)
	{
		pdrgpcrMinimal = CUtils::PdrgpcrRemap(memory_pool, pdrgpcrMinimal, phmulcr, fMustExist);
	}

	DrgPcr *pdrgpcrKeys = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrKeys, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalGbAggDeduplicate(memory_pool, pdrgpcr, pdrgpcrMinimal, Egbaggtype(), pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PcrsStat
//
//	@doc:
//		Compute required stats columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAggDeduplicate::PcrsStat
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CColRefSet *pcrsInput,
	ULONG ulChildIndex
	)
	const
{
	return PcrsStatGbAgg(memory_pool, exprhdl, pcrsInput, ulChildIndex, m_pdrgpcrKeys);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalGbAggDeduplicate::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), CUtils::UlHashColArray(Pdrgpcr()));
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrKeys));

	ULONG ulGbaggtype = (ULONG) Egbaggtype();

	ulHash = gpos::CombineHashes(ulHash, gpos::HashValue<ULONG>(&ulGbaggtype));

	return  gpos::CombineHashes(ulHash, gpos::HashValue<BOOL>(&m_fGeneratesDuplicates));
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalGbAggDeduplicate::PkcDeriveKeys
	(
	IMemoryPool *memory_pool,
	CExpressionHandle & //exprhdl
	)
	const
{
	CKeyCollection *pkc = NULL;

	// Gb produces a key only if it's global
	if (FGlobal())
	{
		// keys from join child are still keys
		m_pdrgpcrKeys->AddRef();
		pkc = GPOS_NEW(memory_pool) CKeyCollection(memory_pool, m_pdrgpcrKeys);
	}

	return pkc;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalGbAggDeduplicate::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalGbAggDeduplicate *popAgg = CLogicalGbAggDeduplicate::PopConvert(pop);

	if (FGeneratesDuplicates() != popAgg->FGeneratesDuplicates())
	{
		return false;
	}

	return popAgg->Egbaggtype() == Egbaggtype() &&
			Pdrgpcr()->Equals(popAgg->Pdrgpcr()) &&
			m_pdrgpcrKeys->Equals(popAgg->PdrgpcrKeys()) &&
			CColRef::Equals(PdrgpcrMinimal(), popAgg->PdrgpcrMinimal());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalGbAggDeduplicate::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfPushGbDedupBelowJoin);
	(void) pxfs->ExchangeSet(CXform::ExfSplitGbAggDedup);
	(void) pxfs->ExchangeSet(CXform::ExfGbAggDedup2HashAggDedup);
	(void) pxfs->ExchangeSet(CXform::ExfGbAggDedup2StreamAggDedup);
	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalGbAggDeduplicate::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	GPOS_ASSERT(Esp(exprhdl) > EspNone);
	IStatistics *pstatsChild = exprhdl.Pstats(0);

	// extract computed columns
	ULongPtrArray *pdrgpulComputedCols = GPOS_NEW(memory_pool) ULongPtrArray(memory_pool);
	exprhdl.Pdpscalar(1 /*ulChildIndex*/)->PcrsDefined()->ExtractColIds(memory_pool, pdrgpulComputedCols);

	// construct bitset with keys of join child
	CBitSet *pbsKeys = GPOS_NEW(memory_pool) CBitSet(memory_pool);
	const ULONG ulKeys = m_pdrgpcrKeys->Size();
	for (ULONG ul = 0; ul < ulKeys; ul++)
	{
		CColRef *pcr = (*m_pdrgpcrKeys)[ul];
		pbsKeys->ExchangeSet(pcr->UlId());
	}

	IStatistics *pstats = CLogicalGbAgg::PstatsDerive(memory_pool, pstatsChild, Pdrgpcr(), pdrgpulComputedCols, pbsKeys);
	pbsKeys->Release();
	pdrgpulComputedCols->Release();

	return pstats;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAggDeduplicate::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalGbAggDeduplicate::OsPrint
	(
	IOstream &os
	)
	const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os	<< SzId()
		<< "( ";
	OsPrintGbAggType(os, Egbaggtype());
	os	<< " )";
	os	<< " Grp Cols: [";
	CUtils::OsPrintDrgPcr(os, Pdrgpcr());
	os	<< "], Minimal Grp Cols: [";
	if (NULL != PdrgpcrMinimal())
	{
		CUtils::OsPrintDrgPcr(os, PdrgpcrMinimal());
	}
	os << "], Join Child Keys: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrKeys);
	os	<< "]";
	os	<< ", Generates Duplicates :[ " << FGeneratesDuplicates() << " ] ";
	
	return os;
}

// EOF
