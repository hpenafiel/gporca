//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalSplit.cpp
//
//	@doc:
//		Implementation of logical split operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/CPartIndexMap.h"

#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalSplit.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::CLogicalSplit
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalSplit::CLogicalSplit
	(
	IMemoryPool *memory_pool
	)
	:
	CLogical(memory_pool),
	m_pdrgpcrDelete(NULL),
	m_pdrgpcrInsert(NULL),
	m_pcrCtid(NULL),
	m_pcrSegmentId(NULL),
	m_pcrAction(NULL),
	m_pcrTupleOid(NULL)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::CLogicalSplit
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalSplit::CLogicalSplit
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcrDelete,
	DrgPcr *pdrgpcrInsert,
	CColRef *pcrCtid,
	CColRef *pcrSegmentId,
	CColRef *pcrAction,
	CColRef *pcrTupleOid
	)
	:
	CLogical(memory_pool),
	m_pdrgpcrDelete(pdrgpcrDelete),
	m_pdrgpcrInsert(pdrgpcrInsert),
	m_pcrCtid(pcrCtid),
	m_pcrSegmentId(pcrSegmentId),
	m_pcrAction(pcrAction),
	m_pcrTupleOid(pcrTupleOid)

{
	GPOS_ASSERT(NULL != pdrgpcrDelete);
	GPOS_ASSERT(NULL != pdrgpcrInsert);
	GPOS_ASSERT(pdrgpcrInsert->Size() == pdrgpcrDelete->Size());
	GPOS_ASSERT(NULL != pcrCtid);
	GPOS_ASSERT(NULL != pcrSegmentId);
	GPOS_ASSERT(NULL != pcrAction);

	m_pcrsLocalUsed->Include(m_pdrgpcrDelete);
	m_pcrsLocalUsed->Include(m_pdrgpcrInsert);
	m_pcrsLocalUsed->Include(m_pcrCtid);
	m_pcrsLocalUsed->Include(m_pcrSegmentId);
	m_pcrsLocalUsed->Include(m_pcrAction);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::~CLogicalSplit
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalSplit::~CLogicalSplit()
{
	CRefCount::SafeRelease(m_pdrgpcrDelete);
	CRefCount::SafeRelease(m_pdrgpcrInsert);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::FMatch
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalSplit::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CLogicalSplit *popSplit = CLogicalSplit::PopConvert(pop);

		return m_pcrCtid == popSplit->PcrCtid() &&
				m_pcrSegmentId == popSplit->PcrSegmentId() &&
				m_pcrAction == popSplit->PcrAction() &&
				m_pcrTupleOid == popSplit->PcrTupleOid() &&
				m_pdrgpcrDelete->Equals(popSplit->PdrgpcrDelete()) &&
				m_pdrgpcrInsert->Equals(popSplit->PdrgpcrInsert());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalSplit::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), CUtils::UlHashColArray(m_pdrgpcrInsert));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrCtid));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrSegmentId));
	ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(m_pcrAction));

	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalSplit::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrDelete = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrDelete, phmulcr, fMustExist);
	DrgPcr *pdrgpcrInsert = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrInsert, phmulcr, fMustExist);
	CColRef *pcrCtid = CUtils::PcrRemap(m_pcrCtid, phmulcr, fMustExist);
	CColRef *pcrSegmentId = CUtils::PcrRemap(m_pcrSegmentId, phmulcr, fMustExist);
	CColRef *pcrAction = CUtils::PcrRemap(m_pcrAction, phmulcr, fMustExist);
	
	CColRef *pcrTupleOid = NULL;
	if (NULL != m_pcrTupleOid)
	{
		pcrTupleOid = CUtils::PcrRemap(m_pcrTupleOid, phmulcr, fMustExist);
	}

	return GPOS_NEW(memory_pool) CLogicalSplit(memory_pool, pdrgpcrDelete, pdrgpcrInsert, pcrCtid, pcrSegmentId, pcrAction, pcrTupleOid);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalSplit::PcrsDeriveOutput
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	GPOS_ASSERT(2 == exprhdl.UlArity());

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	pcrs->Union(exprhdl.Pdprel(0)->PcrsOutput());
	pcrs->Include(m_pcrAction);
	
	if (NULL != m_pcrTupleOid)
	{
		pcrs->Include(m_pcrTupleOid);
	}

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalSplit::PkcDeriveKeys
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalSplit::Maxcard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	// pass on max card of first child
	return exprhdl.Pdprel(0)->Maxcard();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalSplit::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfImplementSplit);
	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalSplit::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	// split returns double the number of tuples coming from its child
	IStatistics *pstats = exprhdl.Pstats(0);

	return pstats->PstatsScale(memory_pool, CDouble(2.0) /*dFactor*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalSplit::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalSplit::OsPrint
	(
	IOstream &os
	)
	const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os	<< SzId() << " -- Delete Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrDelete);
	os	<< "], Insert Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrInsert);
	os	<< "], ";
	m_pcrCtid->OsPrint(os);
	os	<< ", ";
	m_pcrSegmentId->OsPrint(os);
	os	<< ", ";
	m_pcrAction->OsPrint(os);

	return os;
}

// EOF

