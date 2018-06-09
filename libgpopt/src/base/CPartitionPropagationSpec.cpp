//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartitionPropagationSpec.cpp
//
//	@doc:
//		Specification of partition propagation requirements
//---------------------------------------------------------------------------

#include "gpos/memory/CAutoMemoryPool.h"

#include "gpopt/exception.h"

#include "gpopt/base/CPartitionPropagationSpec.h"
#include "gpopt/base/CPartIndexMap.h"
#include "gpopt/operators/CPhysicalPartitionSelector.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpos;
using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::CPartitionPropagationSpec
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartitionPropagationSpec::CPartitionPropagationSpec
	(
	CPartIndexMap *ppim,
	CPartFilterMap *ppfm
	)
	:
	m_ppim(ppim),
	m_ppfm(ppfm)
{
	GPOS_ASSERT(NULL != ppim);
	GPOS_ASSERT(NULL != ppfm);
}


//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::~CPartitionPropagationSpec
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartitionPropagationSpec::~CPartitionPropagationSpec()
{
	m_ppim->Release();
	m_ppfm->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::HashValue
//
//	@doc:
//		Hash of components
//
//---------------------------------------------------------------------------
ULONG
CPartitionPropagationSpec::HashValue() const
{
	return m_ppim->HashValue();
}


//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::FMatch
//
//	@doc:
//		Check whether two partition propagation specs are equal
//
//---------------------------------------------------------------------------
BOOL
CPartitionPropagationSpec::FMatch
	(
	const CPartitionPropagationSpec *ppps
	) 
	const
{
	return m_ppim->Equals(ppps->Ppim()) &&
			m_ppfm->Equals(ppps->m_ppfm);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::AppendEnforcers
//
//	@doc:
//		Add required enforcers to dynamic array
//
//---------------------------------------------------------------------------
void
CPartitionPropagationSpec::AppendEnforcers
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	CReqdPropPlan *
#ifdef GPOS_DEBUG
	prpp
#endif // GPOS_DEBUG
	,
	DrgPexpr *pdrgpexpr, 
	CExpression *pexpr
	)
{
	GPOS_ASSERT(NULL != prpp);
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != pdrgpexpr);
	GPOS_ASSERT(NULL != pexpr);
	
	ULongPtrArray *pdrgpul = m_ppim->PdrgpulScanIds(memory_pool);
	const ULONG size = pdrgpul->Size();
	
	for (ULONG ul = 0; ul < size; ul++)
	{
		ULONG scan_id = *((*pdrgpul)[ul]);
		GPOS_ASSERT(m_ppim->FContains(scan_id));
		
		if (CPartIndexMap::EpimConsumer != m_ppim->Epim(scan_id) || 0 < m_ppim->UlExpectedPropagators(scan_id))
		{
			continue;
		}
		
		if (!FRequiresPartitionPropagation(memory_pool, pexpr, exprhdl, scan_id))
		{
			continue;
		}
		
		CExpression *pexprResolver = NULL;

		IMDId *mdid = m_ppim->GetRelMdId(scan_id);
		DrgDrgPcr *pdrgpdrgpcrKeys = NULL;
		DrgPpartkeys *pdrgppartkeys = m_ppim->Pdrgppartkeys(scan_id);
		CPartConstraint *ppartcnstr = m_ppim->PpartcnstrRel(scan_id);
		PartCnstrMap *ppartcnstrmap = m_ppim->Ppartcnstrmap(scan_id);
		mdid->AddRef();
		ppartcnstr->AddRef();
		ppartcnstrmap->AddRef();
		pexpr->AddRef();
		
		// check if there is a predicate on this part index id
		HMUlExpr *phmulexprEqFilter = GPOS_NEW(memory_pool) HMUlExpr(memory_pool);
		HMUlExpr *phmulexprFilter = GPOS_NEW(memory_pool) HMUlExpr(memory_pool);
		CExpression *pexprResidual = NULL;
		if (m_ppfm->FContainsScanId(scan_id))
		{
			CExpression *pexprScalar = PexprFilter(memory_pool, scan_id);
			
			// find out which keys are used in the predicate, in case there are multiple
			// keys at this point (e.g. from a union of multiple CTE consumers)
			CColRefSet *pcrsUsed = CDrvdPropScalar::Pdpscalar(pexprScalar->PdpDerive())->PcrsUsed();
			const ULONG ulKeysets = pdrgppartkeys->Size();
			for (ULONG ulKey = 0; NULL == pdrgpdrgpcrKeys && ulKey < ulKeysets; ulKey++)
			{
				// get partition key
				CPartKeys *ppartkeys = (*pdrgppartkeys)[ulKey];
				if (ppartkeys->FOverlap(pcrsUsed))
				{
					pdrgpdrgpcrKeys = ppartkeys->Pdrgpdrgpcr();
				}
			}
			
                        // if we cannot find partition keys mapping the partition predicates, fall back to planner
                        if (NULL == pdrgpdrgpcrKeys)
                        {
                            GPOS_RAISE(gpopt::ExmaGPOPT, gpopt::ExmiUnsatisfiedRequiredProperties);
                        }

			pdrgpdrgpcrKeys->AddRef();

			// split predicates and put them in the appropriate hashmaps
			SplitPartPredicates(memory_pool, pexprScalar, pdrgpdrgpcrKeys, phmulexprEqFilter, phmulexprFilter, &pexprResidual);
			pexprScalar->Release();
		}
		else
		{
			// doesn't matter which keys we use here since there is no filter
			GPOS_ASSERT(1 <= pdrgppartkeys->Size());
			pdrgpdrgpcrKeys = (*pdrgppartkeys)[0]->Pdrgpdrgpcr();
			pdrgpdrgpcrKeys->AddRef();
		}

		pexprResolver = GPOS_NEW(memory_pool) CExpression
									(
									memory_pool,
									GPOS_NEW(memory_pool) CPhysicalPartitionSelector
												(
												memory_pool,
												scan_id,
												mdid,
												pdrgpdrgpcrKeys,
												ppartcnstrmap,
												ppartcnstr,
												phmulexprEqFilter,
												phmulexprFilter,
												pexprResidual
												),
									pexpr
									);
		
		pdrgpexpr->Append(pexprResolver);
	}
	pdrgpul->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::PexprFilter
//
//	@doc:
//		Return the filter expression for the given Scan Id
//
//---------------------------------------------------------------------------
CExpression *
CPartitionPropagationSpec::PexprFilter
	(
	IMemoryPool *memory_pool,
	ULONG scan_id
	)
{
	CExpression *pexprScalar = m_ppfm->Pexpr(scan_id);
	GPOS_ASSERT(NULL != pexprScalar);

	if (CUtils::FScalarIdent(pexprScalar))
	{
		// condition of the form "pkey": translate into pkey = true
		pexprScalar->AddRef();
		pexprScalar = CUtils::PexprScalarEqCmp(memory_pool, pexprScalar, CUtils::PexprScalarConstBool(memory_pool, true /*value*/, false /*is_null*/));
	}
	else if (CPredicateUtils::FNot(pexprScalar) && CUtils::FScalarIdent((*pexprScalar)[0]))
	{
		// condition of the form "!pkey": translate into pkey = false
		CExpression *pexprId = (*pexprScalar)[0];
		pexprId->AddRef();

		pexprScalar = CUtils::PexprScalarEqCmp(memory_pool, pexprId, CUtils::PexprScalarConstBool(memory_pool, false /*value*/, false /*is_null*/));
	}
	else
	{
		pexprScalar->AddRef();
	}

	return pexprScalar;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::FRequiresPartitionPropagation
//
//	@doc:
//		Check if given part index id needs to be enforced on top of the given 
//		expression
//
//---------------------------------------------------------------------------
BOOL
CPartitionPropagationSpec::FRequiresPartitionPropagation
	(
	IMemoryPool *memory_pool, 
	CExpression *pexpr, 
	CExpressionHandle &exprhdl,
	ULONG part_idx_id
	)
	const
{
	GPOS_ASSERT(m_ppim->FContains(part_idx_id));
	
	// construct partition propagation spec with the given id only, and check if it needs to be 
	// enforced on top
	CPartIndexMap *ppim = GPOS_NEW(memory_pool) CPartIndexMap(memory_pool);
	
	IMDId *mdid = m_ppim->GetRelMdId(part_idx_id);
	DrgPpartkeys *pdrgppartkeys = m_ppim->Pdrgppartkeys(part_idx_id);
	CPartConstraint *ppartcnstr = m_ppim->PpartcnstrRel(part_idx_id);
	PartCnstrMap *ppartcnstrmap = m_ppim->Ppartcnstrmap(part_idx_id);
	mdid->AddRef();
	pdrgppartkeys->AddRef();
	ppartcnstr->AddRef();
	ppartcnstrmap->AddRef();
	
	ppim->Insert(part_idx_id, ppartcnstrmap, m_ppim->Epim(part_idx_id), m_ppim->UlExpectedPropagators(part_idx_id), mdid, pdrgppartkeys, ppartcnstr);
	
	CPartitionPropagationSpec *ppps = GPOS_NEW(memory_pool) CPartitionPropagationSpec(ppim, GPOS_NEW(memory_pool) CPartFilterMap(memory_pool));
	
	CEnfdPartitionPropagation *pepp = GPOS_NEW(memory_pool) CEnfdPartitionPropagation(ppps, CEnfdPartitionPropagation::EppmSatisfy, GPOS_NEW(memory_pool) CPartFilterMap(memory_pool));
	CEnfdProp::EPropEnforcingType epetPartitionPropagation = pepp->Epet(exprhdl, CPhysical::PopConvert(pexpr->Pop()), true /*fPartitionPropagationRequired*/);
	
	pepp->Release();
	
	return CEnfdProp::FEnforce(epetPartitionPropagation);
}

//---------------------------------------------------------------------------
//      @function:
//		CPartitionPropagationSpec::SplitPartPredicates
//
//	@doc:
//		Split the partition elimination predicates over the various levels
//		as well as the residual predicate and add them to the appropriate
//		hashmaps. These are to be used when creating the partition selector
//
//---------------------------------------------------------------------------
void
CPartitionPropagationSpec::SplitPartPredicates
	(
	IMemoryPool *memory_pool,
	CExpression *pexprScalar,
	DrgDrgPcr *pdrgpdrgpcrKeys,
	HMUlExpr *phmulexprEqFilter,	// output
	HMUlExpr *phmulexprFilter,		// output
	CExpression **ppexprResidual	// output
	)
{
	GPOS_ASSERT(NULL != pexprScalar);
	GPOS_ASSERT(NULL != pdrgpdrgpcrKeys);
	GPOS_ASSERT(NULL != phmulexprEqFilter);
	GPOS_ASSERT(NULL != phmulexprFilter);
	GPOS_ASSERT(NULL != ppexprResidual);
	GPOS_ASSERT(NULL == *ppexprResidual);

	DrgPexpr *pdrgpexprConjuncts = CPredicateUtils::PdrgpexprConjuncts(memory_pool, pexprScalar);
	CBitSet *pbsUsed = GPOS_NEW(memory_pool) CBitSet(memory_pool);
	CColRefSet *pcrsKeys = PcrsKeys(memory_pool, pdrgpdrgpcrKeys);

	const ULONG ulLevels = pdrgpdrgpcrKeys->Size();
	for (ULONG ul = 0; ul < ulLevels; ul++)
	{
		CColRef *pcr = CUtils::PcrExtractPartKey(pdrgpdrgpcrKeys, ul);
		// find conjuncts for this key and mark their positions
		DrgPexpr *pdrgpexprKey = PdrgpexprPredicatesOnKey(memory_pool, pdrgpexprConjuncts, pcr, pcrsKeys, &pbsUsed);
		const ULONG ulLen = pdrgpexprKey->Size();
		if (ulLen == 0)
		{
			// no predicates on this key
			pdrgpexprKey->Release();
			continue;
		}

		if (ulLen == 1 && CPredicateUtils::FIdentCompare((*pdrgpexprKey)[0], IMDType::EcmptEq, pcr))
		{
			// EqFilters
			// one equality predicate (key = expr); take out the expression
			// and add it to the equality filters map
			CExpression *pexprPartKey = NULL;
			CExpression *pexprOther = NULL;
			IMDType::ECmpType cmp_type = IMDType::EcmptOther;

			CPredicateUtils::ExtractComponents((*pdrgpexprKey)[0], pcr, &pexprPartKey, &pexprOther, &cmp_type);
			GPOS_ASSERT(NULL != pexprOther);
			pexprOther->AddRef();
#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
			phmulexprEqFilter->Insert(GPOS_NEW(memory_pool) ULONG(ul), pexprOther);
			GPOS_ASSERT(fResult);
			pdrgpexprKey->Release();
		}
		else
		{
			// Filters
			// more than one predicate on this key or one non-simple-equality predicate
#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
			phmulexprFilter->Insert(GPOS_NEW(memory_pool) ULONG(ul), CPredicateUtils::PexprConjunction(memory_pool, pdrgpexprKey));
			GPOS_ASSERT(fResult);
			continue;
		}

	}

	(*ppexprResidual) = PexprResidualFilter(memory_pool, pdrgpexprConjuncts, pbsUsed);

	pcrsKeys->Release();
	pdrgpexprConjuncts->Release();
	pbsUsed->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::PcrsKeys
//
//	@doc:
//		Return a colrefset containing all the part keys
//
//---------------------------------------------------------------------------
CColRefSet *
CPartitionPropagationSpec::PcrsKeys
	(
	IMemoryPool *memory_pool,
	DrgDrgPcr *pdrgpdrgpcrKeys
	)
{
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	const ULONG ulLevels = pdrgpdrgpcrKeys->Size();
	for (ULONG ul = 0; ul < ulLevels; ul++)
	{
		CColRef *pcr = CUtils::PcrExtractPartKey(pdrgpdrgpcrKeys, ul);
		pcrs->Include(pcr);
	}

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::PexprResidualFilter
//
//	@doc:
//		Return a residual filter given an array of predicates and a bitset
//		indicating which predicates have already been used
//
//---------------------------------------------------------------------------
CExpression *
CPartitionPropagationSpec::PexprResidualFilter
	(
	IMemoryPool *memory_pool,
	DrgPexpr *pdrgpexpr,
	CBitSet *pbsUsed
	)
{
	GPOS_ASSERT(NULL != pdrgpexpr);
	GPOS_ASSERT(NULL != pbsUsed);

	DrgPexpr *pdrgpexprUnused = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);

	const ULONG ulLen = pdrgpexpr->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		if (pbsUsed->Get(ul))
		{
			// predicate already considered
			continue;
		}

		CExpression *pexpr = (*pdrgpexpr)[ul];
		pexpr->AddRef();
		pdrgpexprUnused->Append(pexpr);
	}

	CExpression *pexprResult = CPredicateUtils::PexprConjunction(memory_pool, pdrgpexprUnused);
	if (CUtils::FScalarConstTrue(pexprResult))
	{
		pexprResult->Release();
		pexprResult = NULL;
	}

	return pexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::PdrgpexprPredicatesOnKey
//
//	@doc:
//		Returns an array of predicates on the given partitioning key given
//		an array of predicates on all keys
//
//---------------------------------------------------------------------------
DrgPexpr *
CPartitionPropagationSpec::PdrgpexprPredicatesOnKey
	(
	IMemoryPool *memory_pool,
	DrgPexpr *pdrgpexpr,
	CColRef *pcr,
	CColRefSet *pcrsKeys,
	CBitSet **ppbs
	)
{
	GPOS_ASSERT(NULL != pdrgpexpr);
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(NULL != ppbs);
	GPOS_ASSERT(NULL != *ppbs);

	DrgPexpr *pdrgpexprResult = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);

	const ULONG ulLen = pdrgpexpr->Size();
	for (ULONG ul = 0; ul < ulLen; ul++)
	{
		if ((*ppbs)->Get(ul))
		{
			// this expression has already been added for another column
			continue;
		}

		CExpression *pexpr = (*pdrgpexpr)[ul];
		GPOS_ASSERT(pexpr->Pop()->FScalar());

		CColRefSet *pcrsUsed = CDrvdPropScalar::Pdpscalar(pexpr->PdpDerive())->PcrsUsed();
		CColRefSet *pcrsUsedKeys = GPOS_NEW(memory_pool) CColRefSet(memory_pool, *pcrsUsed);
		pcrsUsedKeys->Intersection(pcrsKeys);

		if (1 == pcrsUsedKeys->Size() && pcrsUsedKeys->FMember(pcr))
		{
			pexpr->AddRef();
			pdrgpexprResult->Append(pexpr);
			(*ppbs)->ExchangeSet(ul);
		}

		pcrsUsedKeys->Release();
	}

	return pdrgpexprResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartitionPropagationSpec::OsPrint
//
//	@doc:
//		Print function
//
//---------------------------------------------------------------------------
IOstream &
CPartitionPropagationSpec::OsPrint
	(
	IOstream &os
	)
	const
{
	os << *m_ppim;
	
	os << "Filters: [";
	m_ppfm->OsPrint(os);
	os << "]";
	return os;
}


// EOF

