//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogicalGbAgg.cpp
//
//	@doc:
//		Implementation of aggregate operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalGbAgg.h"

#include "naucrates/statistics/CGroupByStatsProcessor.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::CLogicalGbAgg
//
//	@doc:
//		ctor for xform pattern
//
//---------------------------------------------------------------------------
CLogicalGbAgg::CLogicalGbAgg
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalUnary(memory_pool),
	m_pdrgpcr(NULL),
	m_pdrgpcrMinimal(NULL),
	m_egbaggtype(COperator::EgbaggtypeSentinel),
	m_fGeneratesDuplicates(true),
	m_pdrgpcrArgDQA(NULL)
{
	m_fPattern = true;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::CLogicalGbAgg
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalGbAgg::CLogicalGbAgg
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr,
	COperator::EGbAggType egbaggtype
	)
	:
	CLogicalUnary(memory_pool),
	m_pdrgpcr(pdrgpcr),
	m_pdrgpcrMinimal(NULL),
	m_egbaggtype(egbaggtype),
	m_fGeneratesDuplicates(false),
	m_pdrgpcrArgDQA(NULL)
{
	if (COperator::EgbaggtypeLocal == egbaggtype)
	{
		// final and intermediate aggregates have to remove duplicates for a given group
		m_fGeneratesDuplicates = true;
	}

	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(COperator::EgbaggtypeSentinel > egbaggtype);
	GPOS_ASSERT(COperator::EgbaggtypeIntermediate != egbaggtype);

	m_pcrsLocalUsed->Include(m_pdrgpcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::CLogicalGbAgg
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalGbAgg::CLogicalGbAgg
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr,
	COperator::EGbAggType egbaggtype,
	BOOL fGeneratesDuplicates,
	DrgPcr *pdrgpcrArgDQA
	)
	:
	CLogicalUnary(memory_pool),
	m_pdrgpcr(pdrgpcr),
	m_pdrgpcrMinimal(NULL),
	m_egbaggtype(egbaggtype),
	m_fGeneratesDuplicates(fGeneratesDuplicates),
	m_pdrgpcrArgDQA(pdrgpcrArgDQA)
{
	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(COperator::EgbaggtypeSentinel > egbaggtype);
	GPOS_ASSERT_IMP(NULL == m_pdrgpcrArgDQA, COperator::EgbaggtypeIntermediate != egbaggtype);
	GPOS_ASSERT_IMP(m_fGeneratesDuplicates, COperator::EgbaggtypeLocal == egbaggtype);

	m_pcrsLocalUsed->Include(m_pdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::CLogicalGbAgg
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalGbAgg::CLogicalGbAgg
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr,
	DrgPcr *pdrgpcrMinimal,
	COperator::EGbAggType egbaggtype
	)
	:
	CLogicalUnary(memory_pool),
	m_pdrgpcr(pdrgpcr),
	m_pdrgpcrMinimal(pdrgpcrMinimal),
	m_egbaggtype(egbaggtype),
	m_fGeneratesDuplicates(true),
	m_pdrgpcrArgDQA(NULL)
{
	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(COperator::EgbaggtypeSentinel > egbaggtype);
	GPOS_ASSERT(COperator::EgbaggtypeIntermediate != egbaggtype);

	GPOS_ASSERT_IMP(NULL != pdrgpcrMinimal, pdrgpcrMinimal->Size() <= pdrgpcr->Size());

	if (NULL == pdrgpcrMinimal)
	{
		m_pdrgpcr->AddRef();
		m_pdrgpcrMinimal = m_pdrgpcr;
	}

	m_pcrsLocalUsed->Include(m_pdrgpcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::CLogicalGbAgg
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalGbAgg::CLogicalGbAgg
	(
	IMemoryPool *memory_pool,
	DrgPcr *pdrgpcr,
	DrgPcr *pdrgpcrMinimal,
	COperator::EGbAggType egbaggtype,
	BOOL fGeneratesDuplicates,
	DrgPcr *pdrgpcrArgDQA
	)
	:
	CLogicalUnary(memory_pool),
	m_pdrgpcr(pdrgpcr),
	m_pdrgpcrMinimal(pdrgpcrMinimal),
	m_egbaggtype(egbaggtype),
	m_fGeneratesDuplicates(fGeneratesDuplicates),
	m_pdrgpcrArgDQA(pdrgpcrArgDQA)
{
	GPOS_ASSERT(NULL != pdrgpcr);
	GPOS_ASSERT(COperator::EgbaggtypeSentinel > egbaggtype);

	GPOS_ASSERT_IMP(NULL != pdrgpcrMinimal, pdrgpcrMinimal->Size() <= pdrgpcr->Size());
	GPOS_ASSERT_IMP(NULL == m_pdrgpcrArgDQA, COperator::EgbaggtypeIntermediate != egbaggtype);
	GPOS_ASSERT_IMP(m_fGeneratesDuplicates, COperator::EgbaggtypeLocal == egbaggtype);

	if (NULL == pdrgpcrMinimal)
	{
		m_pdrgpcr->AddRef();
		m_pdrgpcrMinimal = m_pdrgpcr;
	}

	m_pcrsLocalUsed->Include(m_pdrgpcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::~CLogicalGbAgg
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CLogicalGbAgg::~CLogicalGbAgg()
{
	// safe release -- to allow for instances used in patterns
	CRefCount::SafeRelease(m_pdrgpcr);
	CRefCount::SafeRelease(m_pdrgpcrMinimal);
	CRefCount::SafeRelease(m_pdrgpcrArgDQA);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalGbAgg::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcr = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcr, phmulcr, fMustExist);
	DrgPcr *pdrgpcrMinimal = NULL;
	if (NULL != m_pdrgpcrMinimal)
	{
		pdrgpcrMinimal = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrMinimal, phmulcr, fMustExist);
	}

	DrgPcr *pdrgpcrArgDQA = NULL;
	if (NULL != m_pdrgpcrArgDQA)
	{
		pdrgpcrArgDQA = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrArgDQA, phmulcr, fMustExist);
	}

	return GPOS_NEW(memory_pool) CLogicalGbAgg(memory_pool, pdrgpcr, pdrgpcrMinimal, Egbaggtype(), m_fGeneratesDuplicates, pdrgpcrArgDQA);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAgg::PcrsDeriveOutput
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	
	// include the intersection of the grouping columns and the child's output
	pcrs->Include(Pdrgpcr());
	CDrvdPropRelational *pdprel = exprhdl.Pdprel(0);
	pcrs->Intersection(pdprel->PcrsOutput());
	
	// the scalar child defines additional columns
	pcrs->Union(exprhdl.Pdpscalar(1 /*ulChildIndex*/)->PcrsDefined());
	
	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PcrsDeriveOuter
//
//	@doc:
//		Derive outer references
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAgg::PcrsDeriveOuter
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	CColRefSet *pcrsGrp = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	pcrsGrp->Include(m_pdrgpcr);

	CColRefSet *pcrsOuter = CLogical::PcrsDeriveOuter(memory_pool, exprhdl, pcrsGrp);
	pcrsGrp->Release();

	return pcrsOuter;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PpcDeriveConstraint
//
//	@doc:
//		Derive constraint property
//
//---------------------------------------------------------------------------
CPropConstraint *
CLogicalGbAgg::PpcDeriveConstraint
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	CColRefSet *pcrsGrouping = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	pcrsGrouping->Include(m_pdrgpcr);

	// get the constraints on the grouping columns only
	CPropConstraint *ppc = PpcDeriveConstraintRestrict(memory_pool, exprhdl, pcrsGrouping);
	pcrsGrouping->Release();

	return ppc;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PcrsStat
//
//	@doc:
//		Compute required stats columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAgg::PcrsStat
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CColRefSet *pcrsInput,
	ULONG ulChildIndex
	)
	const
{
	return PcrsStatGbAgg(memory_pool, exprhdl, pcrsInput, ulChildIndex, m_pdrgpcr);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PcrsStatGbAgg
//
//	@doc:
//		Compute required stats columns for a GbAgg
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAgg::PcrsStatGbAgg
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CColRefSet *pcrsInput,
	ULONG ulChildIndex,
	DrgPcr *pdrgpcrGrp
	)
	const
{
	GPOS_ASSERT(NULL != pdrgpcrGrp);
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	// include grouping columns
	pcrs->Include(pdrgpcrGrp);

	// other columns used in aggregates
	pcrs->Union(exprhdl.Pdpscalar(1 /*ulChildIndex*/)->PcrsUsed());

	// if the grouping column is a computed column, then add its corresponding used columns
	// to required columns for statistics computation
	CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
	const ULONG ulGrpCols = m_pdrgpcr->Size();
	for (ULONG ul = 0; ul < ulGrpCols; ul++)
	{
		CColRef *pcrGrpCol = (*m_pdrgpcr)[ul];
		const CColRefSet *pcrsUsed = pcf->PcrsUsedInComputedCol(pcrGrpCol);
		if (NULL != pcrsUsed)
		{
			pcrs->Union(pcrsUsed);
		}
	}

	CColRefSet *pcrsRequired = PcrsReqdChildStats(memory_pool, exprhdl, pcrsInput, pcrs, ulChildIndex);
	pcrs->Release();

	return pcrsRequired;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PcrsDeriveNotNull
//
//	@doc:
//		Derive not null columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalGbAgg::PcrsDeriveNotNull
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	// include grouping columns
	pcrs->Include(Pdrgpcr());

	// intersect with not nullable columns from relational child
	pcrs->Intersection(exprhdl.Pdprel(0)->PcrsNotNull());

	// TODO,  03/18/2012, add nullability info of computed columns

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalGbAgg::HashValue() const
{
	ULONG ulHash = COperator::HashValue();
	ULONG ulArity = m_pdrgpcr->Size();
	ULONG ulGbaggtype = (ULONG) m_egbaggtype;

	for (ULONG ul = 0; ul < ulArity; ul++)
	{
		CColRef *pcr = (*m_pdrgpcr)[ul];
		ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(pcr));
	}

	ulHash = gpos::CombineHashes(ulHash, gpos::HashValue<ULONG>(&ulGbaggtype));

	return  gpos::CombineHashes(ulHash, gpos::HashValue<BOOL>(&m_fGeneratesDuplicates));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalGbAgg::PkcDeriveKeys
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	CKeyCollection *pkc = NULL;

	// Gb produces a key only if it's global
	if (FGlobal())
	{
		if (COperator::EgbaggtypeLocal == m_egbaggtype && !m_fGeneratesDuplicates)
		{
			return pkc;
		}

		if (0 < m_pdrgpcr->Size())
		{
			// grouping columns always constitute a key
			m_pdrgpcr->AddRef();
			pkc = GPOS_NEW(memory_pool) CKeyCollection(memory_pool, m_pdrgpcr);
		}
		else
		{
			// scalar and single-group aggs produce one row that constitutes a key
			CColRefSet *pcrs = exprhdl.Pdpscalar(1)->PcrsDefined();

			if (0 == pcrs->Size())
			{ 
				// aggregate defines no columns, e.g. select 1 from r group by a
				return NULL;
			}
			
			pcrs->AddRef();
			pkc = GPOS_NEW(memory_pool) CKeyCollection(memory_pool, pcrs);
		}
	}

	return pkc;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalGbAgg::Maxcard
	(
	IMemoryPool *, //memory_pool
	CExpressionHandle &exprhdl
	)
	const
{
	// agg w/o grouping columns produces one row
	if (0 == m_pdrgpcr->Size())
	{
		return CMaxCard(1 /*ull*/);
	}
	
	// contradictions produce no rows
	if (CDrvdPropRelational::Pdprel(exprhdl.Pdp())->Ppc()->FContradiction())
	{
		return CMaxCard(0 /*ull*/);
	}

	return CMaxCard();
}	
	
	
//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalGbAgg::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalGbAgg *popAgg = reinterpret_cast<CLogicalGbAgg*> (pop);

	return FGeneratesDuplicates() == popAgg->FGeneratesDuplicates() &&
			popAgg->Egbaggtype() == m_egbaggtype &&
			CColRef::Equals(m_pdrgpcr, popAgg->m_pdrgpcr) &&
			CColRef::Equals(m_pdrgpcrMinimal, popAgg->PdrgpcrMinimal()) &&
			CColRef::Equals(m_pdrgpcrArgDQA, popAgg->PdrgpcrArgDQA());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalGbAgg::PxfsCandidates
	(
	IMemoryPool *memory_pool
	) 
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	
	(void) pxfs->ExchangeSet(CXform::ExfSimplifyGbAgg);
	(void) pxfs->ExchangeSet(CXform::ExfGbAggWithMDQA2Join);
	(void) pxfs->ExchangeSet(CXform::ExfCollapseGbAgg);
	(void) pxfs->ExchangeSet(CXform::ExfPushGbBelowJoin);
	(void) pxfs->ExchangeSet(CXform::ExfPushGbBelowUnion);
	(void) pxfs->ExchangeSet(CXform::ExfPushGbBelowUnionAll);
	(void) pxfs->ExchangeSet(CXform::ExfSplitGbAgg);
	(void) pxfs->ExchangeSet(CXform::ExfSplitDQA);
	(void) pxfs->ExchangeSet(CXform::ExfGbAgg2Apply);
	(void) pxfs->ExchangeSet(CXform::ExfGbAgg2HashAgg);
	(void) pxfs->ExchangeSet(CXform::ExfGbAgg2StreamAgg);
	(void) pxfs->ExchangeSet(CXform::ExfGbAgg2ScalarAgg);

	return pxfs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalGbAgg::PstatsDerive
	(
	IMemoryPool *memory_pool,
	IStatistics *pstatsChild,
	DrgPcr *pdrgpcrGroupingCols,
	ULongPtrArray *pdrgpulComputedCols,
	CBitSet *pbsKeys
	)
{
	const ULONG ulGroupingCols = pdrgpcrGroupingCols->Size();

	// extract grouping column ids
	ULongPtrArray *pdrgpulGroupingCols = GPOS_NEW(memory_pool) ULongPtrArray(memory_pool);
	for (ULONG ul = 0; ul < ulGroupingCols; ul++)
	{
		CColRef *pcr = (*pdrgpcrGroupingCols)[ul];
		pdrgpulGroupingCols->Append(GPOS_NEW(memory_pool) ULONG(pcr->UlId()));
	}

	IStatistics *pstats = CGroupByStatsProcessor::PstatsGroupBy(memory_pool, dynamic_cast<CStatistics *>(pstatsChild), pdrgpulGroupingCols, pdrgpulComputedCols, pbsKeys);

	// clean up
	pdrgpulGroupingCols->Release();

	return pstats;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalGbAgg::PstatsDerive
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

	IStatistics *pstats = PstatsDerive(memory_pool, pstatsChild, Pdrgpcr(), pdrgpulComputedCols, NULL /*pbsKeys*/);

	pdrgpulComputedCols->Release();

	return pstats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalGbAgg::OsPrint
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
	OsPrintGbAggType(os, m_egbaggtype);
	os	<< " )";
	os	<< " Grp Cols: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcr);
	os	<< "]"
		<< "[";
	OsPrintGbAggType(os, m_egbaggtype);
	os	<< "]";

	os << ", Minimal Grp Cols: [";
	if (NULL != m_pdrgpcrMinimal)
	{
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrMinimal);
	}
	os << "]";
	
	if (COperator::EgbaggtypeIntermediate == m_egbaggtype)
	{
		os	<< ", Distinct Cols:[";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrArgDQA);
		os	<< "]";
	}
	os	<< ", Generates Duplicates :[ " << FGeneratesDuplicates() << " ] ";

	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalGbAgg::OsPrintGbAggType
//
//	@doc:
//		Helper function to print aggregate type
//
//---------------------------------------------------------------------------
IOstream &
CLogicalGbAgg::OsPrintGbAggType
	(
	IOstream &os,
	COperator::EGbAggType egbaggtype
	)
{
	switch (egbaggtype)
	{
		case COperator::EgbaggtypeGlobal:
				os << "Global";
				break;

		case COperator::EgbaggtypeIntermediate:
				os << "Intermediate";
				break;

		case COperator::EgbaggtypeLocal:
				os << "Local";
				break;

		default:
			GPOS_ASSERT(!"Unsupported aggregate type");
	}
	return os;
}

// EOF
