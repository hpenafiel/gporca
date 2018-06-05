//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartConstraint.cpp
//
//	@doc:
//		Implementation of part constraints
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CConstraint.h"
#include "gpopt/base/CConstraintNegation.h"
#include "gpopt/base/CConstraintConjunction.h"
#include "gpopt/base/CUtils.h"

#include "gpopt/metadata/CPartConstraint.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::CPartConstraint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartConstraint::CPartConstraint
	(
	IMemoryPool *memory_pool,
	HMUlCnstr *phmulcnstr,
	CBitSet *pbsDefaultParts,
	BOOL fUnbounded,
	DrgDrgPcr *pdrgpdrgpcr
	)
	:
	m_phmulcnstr(phmulcnstr),
	m_pbsDefaultParts(pbsDefaultParts),
	m_fUnbounded(fUnbounded),
	m_fUninterpreted(false),
	m_pdrgpdrgpcr(pdrgpdrgpcr)
{
	GPOS_ASSERT(NULL != phmulcnstr);
	GPOS_ASSERT(NULL != pbsDefaultParts);
	GPOS_ASSERT(NULL != pdrgpdrgpcr);
	m_num_of_part_levels = pdrgpdrgpcr->Size();
	GPOS_ASSERT_IMP(fUnbounded, FAllDefaultPartsIncluded());

	m_pcnstrCombined = PcnstrBuildCombined(memory_pool);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::CPartConstraint
//
//	@doc:
//		Ctor - shortcut for single-level
//
//---------------------------------------------------------------------------
CPartConstraint::CPartConstraint
	(
	IMemoryPool *memory_pool,
	CConstraint *pcnstr,
	BOOL fDefaultPartition,
	BOOL fUnbounded
	)
	:
	m_phmulcnstr(NULL),
	m_pbsDefaultParts(NULL),
	m_fUnbounded(fUnbounded),
	m_fUninterpreted(false)
{
	GPOS_ASSERT(NULL != pcnstr);
	GPOS_ASSERT_IMP(fUnbounded, fDefaultPartition);

	m_phmulcnstr = GPOS_NEW(memory_pool) HMUlCnstr(memory_pool);
#ifdef GPOS_DEBUG
	BOOL fResult =
#endif // GPOS_DEBUG
	m_phmulcnstr->Insert(GPOS_NEW(memory_pool) ULONG(0 /*ulLevel*/), pcnstr);
	GPOS_ASSERT(fResult);

	CColRefSet *pcrsUsed = pcnstr->PcrsUsed();
	GPOS_ASSERT(1 == pcrsUsed->Size());
	CColRef *pcrPartKey = pcrsUsed->PcrFirst();

	DrgPcr *pdrgpcr = GPOS_NEW(memory_pool) DrgPcr(memory_pool);
	pdrgpcr->Append(pcrPartKey);

	m_pdrgpdrgpcr = GPOS_NEW(memory_pool) DrgDrgPcr(memory_pool);
	m_pdrgpdrgpcr->Append(pdrgpcr);

	m_num_of_part_levels = 1;
	m_pbsDefaultParts = GPOS_NEW(memory_pool) CBitSet(memory_pool);
	if (fDefaultPartition)
	{
		m_pbsDefaultParts->ExchangeSet(0 /*ulBit*/);
	}

	pcnstr->AddRef();
	m_pcnstrCombined = pcnstr;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::CPartConstraint
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPartConstraint::CPartConstraint
	(
	BOOL fUninterpreted
	)
	:
	m_phmulcnstr(NULL),
	m_pbsDefaultParts(NULL),
	m_num_of_part_levels(1),
	m_fUnbounded(false),
	m_fUninterpreted(fUninterpreted),
	m_pdrgpdrgpcr(NULL),
	m_pcnstrCombined(NULL)
{
	GPOS_ASSERT(fUninterpreted);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::~CPartConstraint
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPartConstraint::~CPartConstraint()
{
	CRefCount::SafeRelease(m_phmulcnstr);
	CRefCount::SafeRelease(m_pbsDefaultParts);
	CRefCount::SafeRelease(m_pdrgpdrgpcr);
	CRefCount::SafeRelease(m_pcnstrCombined);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::PcnstrBuildCombined
//
//	@doc:
//		Construct the combined constraint
//
//---------------------------------------------------------------------------
CConstraint *
CPartConstraint::PcnstrBuildCombined
	(
	IMemoryPool *memory_pool
	)
{
	DrgPcnstr *pdrgpcnstr = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		CConstraint *pcnstr = m_phmulcnstr->Find(&ul);
		if (NULL != pcnstr)
		{
			pcnstr->AddRef();
			pdrgpcnstr->Append(pcnstr);
		}
	}

	return CConstraint::PcnstrConjunction(memory_pool, pdrgpcnstr);
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FAllDefaultPartsIncluded
//
//	@doc:
//		Are all default partitions on all levels included
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FAllDefaultPartsIncluded()
{
	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		if (!FDefaultPartition(ul))
		{
			return false;
		}
	}

	return true;
}
#endif //GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FUnbounded
//
//	@doc:
//		Is part constraint unbounded
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FUnbounded() const
{
	return m_fUnbounded;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FEquivalent
//
//	@doc:
//		Are constraints equivalent
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FEquivalent
	(
	const CPartConstraint *ppartcnstr
	)
	const
{
	GPOS_ASSERT(NULL != ppartcnstr);

	if (m_fUninterpreted || ppartcnstr->FUninterpreted())
	{
		return m_fUninterpreted && ppartcnstr->FUninterpreted();
	}

	if (FUnbounded())
	{
		return ppartcnstr->FUnbounded();
	}
	
	return m_num_of_part_levels == ppartcnstr->m_num_of_part_levels &&
			m_pbsDefaultParts->Equals(ppartcnstr->m_pbsDefaultParts) &&
			FEqualConstrMaps(m_phmulcnstr, ppartcnstr->m_phmulcnstr, m_num_of_part_levels);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FEqualConstrMaps
//
//	@doc:
//		Check if two constaint maps have the same constraints
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FEqualConstrMaps
	(
	HMUlCnstr *phmulcnstrFst,
	HMUlCnstr *phmulcnstrSnd,
	ULONG ulLevels
	)
{
	if (phmulcnstrFst->Size() != phmulcnstrSnd->Size())
	{
		return false;
	}

	for (ULONG ul = 0; ul < ulLevels; ul++)
	{
		CConstraint *pcnstrFst = phmulcnstrFst->Find(&ul);
		CConstraint *pcnstrSnd = phmulcnstrSnd->Find(&ul);

		if ((NULL == pcnstrFst || NULL == pcnstrSnd) && pcnstrFst != pcnstrSnd)
		{
			return false;
		}

		if (NULL != pcnstrFst && !pcnstrFst->Equals(pcnstrSnd))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::Pcnstr
//
//	@doc:
//		Constraint at given level
//
//---------------------------------------------------------------------------
CConstraint *
CPartConstraint::Pcnstr
	(
	ULONG ulLevel
	)
	const
{
	GPOS_ASSERT(!m_fUninterpreted && "Calling Pcnstr on uninterpreted partition constraint");
	return m_phmulcnstr->Find(&ulLevel);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FOverlapLevel
//
//	@doc:
//		Does the current constraint overlap with given one at the given level
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FOverlapLevel
	(
	IMemoryPool *memory_pool,
	const CPartConstraint *ppartcnstr,
	ULONG ulLevel
	)
	const
{
	GPOS_ASSERT(NULL != ppartcnstr);
	GPOS_ASSERT(!FUnbounded());
	GPOS_ASSERT(!ppartcnstr->FUnbounded());

	DrgPcnstr *pdrgpcnstr = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	CConstraint *pcnstrCurrent = Pcnstr(ulLevel);
	CConstraint *pcnstrOther = ppartcnstr->Pcnstr(ulLevel);
	GPOS_ASSERT(NULL != pcnstrCurrent);
	GPOS_ASSERT(NULL != pcnstrOther);

	pcnstrCurrent->AddRef();
	pcnstrOther->AddRef();
	pdrgpcnstr->Append(pcnstrCurrent);
	pdrgpcnstr->Append(pcnstrOther);

	CConstraint *pcnstrIntersect = CConstraint::PcnstrConjunction(memory_pool, pdrgpcnstr);

	BOOL fOverlap = !pcnstrIntersect->FContradiction();
	pcnstrIntersect->Release();

	return fOverlap || (FDefaultPartition(ulLevel) && ppartcnstr->FDefaultPartition(ulLevel));
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FOverlap
//
//	@doc:
//		Does constraint overlap with given one
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FOverlap
	(
	IMemoryPool *memory_pool,
	const CPartConstraint *ppartcnstr
	)
	const
{
	GPOS_ASSERT(NULL != ppartcnstr);
	GPOS_ASSERT(!m_fUninterpreted && "Calling FOverlap on uninterpreted partition constraint");
	
	if (FUnbounded() || ppartcnstr->FUnbounded())
	{
		return true;
	}
	
	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		if (!FOverlapLevel(memory_pool, ppartcnstr, ul))
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FSubsume
//
//	@doc:
//		Does constraint subsume given one
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FSubsume
	(
	const CPartConstraint *ppartcnstr
	)
	const
{
	GPOS_ASSERT(NULL != ppartcnstr);
	GPOS_ASSERT(!m_fUninterpreted && "Calling FSubsume on uninterpreted partition constraint");

	if (FUnbounded())
	{
		return true;
	}

	if (ppartcnstr->FUnbounded())
	{
		return false;
	}

	BOOL fSubsumeLevel = true;
	for (ULONG ul = 0; ul < m_num_of_part_levels && fSubsumeLevel; ul++)
	{
		CConstraint *pcnstrCurrent = Pcnstr(ul);
		CConstraint *pcnstrOther = ppartcnstr->Pcnstr(ul);
		GPOS_ASSERT(NULL != pcnstrCurrent);
		GPOS_ASSERT(NULL != pcnstrOther);

		fSubsumeLevel = pcnstrCurrent->FContains(pcnstrOther) &&
						(FDefaultPartition(ul) || !ppartcnstr->FDefaultPartition(ul));
	}

	return fSubsumeLevel;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FCanNegate
//
//	@doc:
//		Check whether or not the current part constraint can be negated. A part
//		constraint can be negated only if it has constraints on the first level
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FCanNegate() const
{
	// first level cannot be NULL
	if (NULL == Pcnstr(0))
	{
		return false;
	}

	// all levels after the first must be unconstrained
	for (ULONG ul = 1; ul < m_num_of_part_levels; ul++)
	{
		CConstraint *pcnstr = Pcnstr(ul);
		if (NULL == pcnstr || !pcnstr->FUnbounded())
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::PpartcnstrRemaining
//
//	@doc:
//		Return what remains of the current part constraint after taking out
//		the given part constraint. Returns NULL is the difference cannot be
//		performed
//
//---------------------------------------------------------------------------
CPartConstraint *
CPartConstraint::PpartcnstrRemaining
	(
	IMemoryPool *memory_pool,
	CPartConstraint *ppartcnstr
	)
{
	GPOS_ASSERT(!m_fUninterpreted && "Calling PpartcnstrRemaining on uninterpreted partition constraint");
	GPOS_ASSERT(NULL != ppartcnstr);

	if (m_num_of_part_levels != ppartcnstr->m_num_of_part_levels || !ppartcnstr->FCanNegate())
	{
		return NULL;
	}

	HMUlCnstr *phmulcnstr = GPOS_NEW(memory_pool) HMUlCnstr(memory_pool);
	CBitSet *pbsDefaultParts = GPOS_NEW(memory_pool) CBitSet(memory_pool);

	// constraint on first level
	CConstraint *pcnstrCurrent = Pcnstr(0 /*ulLevel*/);
	CConstraint *pcnstrOther = ppartcnstr->Pcnstr(0 /*ulLevel*/);

	CConstraint *pcnstrRemaining = PcnstrRemaining(memory_pool, pcnstrCurrent, pcnstrOther);

#ifdef GPOS_DEBUG
	BOOL fResult =
#endif // GPOS_DEBUG
	phmulcnstr->Insert(GPOS_NEW(memory_pool) ULONG(0), pcnstrRemaining);
	GPOS_ASSERT(fResult);

	if (FDefaultPartition(0 /*ulLevel*/) && !ppartcnstr->FDefaultPartition(0 /*ulLevel*/))
	{
		pbsDefaultParts->ExchangeSet(0 /*ulBit*/);
	}

	// copy the remaining constraints and default partition flags
	for (ULONG ul = 1; ul < m_num_of_part_levels; ul++)
	{
		CConstraint *pcnstrLevel = Pcnstr(ul);
		if (NULL != pcnstrLevel)
		{
			pcnstrLevel->AddRef();
#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
			phmulcnstr->Insert(GPOS_NEW(memory_pool) ULONG(ul), pcnstrLevel);
			GPOS_ASSERT(fResult);
		}

		if (FDefaultPartition(ul))
		{
			pbsDefaultParts->ExchangeSet(ul);
		}
	}

	m_pdrgpdrgpcr->AddRef();
	return GPOS_NEW(memory_pool) CPartConstraint(memory_pool, phmulcnstr, pbsDefaultParts, false /*fUnbounded*/, m_pdrgpdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::PcnstrRemaining
//
//	@doc:
//		Return the remaining part of the first constraint that is not covered by
//		the second constraint
//
//---------------------------------------------------------------------------
CConstraint *
CPartConstraint::PcnstrRemaining
	(
	IMemoryPool *memory_pool,
	CConstraint *pcnstrFst,
	CConstraint *pcnstrSnd
	)
{
	GPOS_ASSERT(NULL != pcnstrSnd);

	pcnstrSnd->AddRef();
	CConstraint *pcnstrNegation = GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, pcnstrSnd);

	if (NULL == pcnstrFst || pcnstrFst->FUnbounded())
	{
		return pcnstrNegation;
	}

	DrgPcnstr *pdrgpcnstr = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	pcnstrFst->AddRef();
	pdrgpcnstr->Append(pcnstrFst);
	pdrgpcnstr->Append(pcnstrNegation);

	return GPOS_NEW(memory_pool) CConstraintConjunction(memory_pool, pdrgpcnstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::PpartcnstrCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the part constraint with remapped columns
//
//---------------------------------------------------------------------------
CPartConstraint *
CPartConstraint::PpartcnstrCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	if (m_fUninterpreted)
	{
		return GPOS_NEW(memory_pool) CPartConstraint(true /*m_fUninterpreted*/);
	}

	HMUlCnstr *phmulcnstr = GPOS_NEW(memory_pool) HMUlCnstr(memory_pool);
	DrgDrgPcr *pdrgpdrgpcr = GPOS_NEW(memory_pool) DrgDrgPcr(memory_pool);

	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		DrgPcr *pdrgpcr = (*m_pdrgpdrgpcr)[ul];
		DrgPcr *pdrgpcrMapped = CUtils::PdrgpcrRemap(memory_pool, pdrgpcr, phmulcr, fMustExist);
		pdrgpdrgpcr->Append(pdrgpcrMapped);

		CConstraint *pcnstr = Pcnstr(ul);
		if (NULL != pcnstr)
		{
			CConstraint *pcnstrRemapped = pcnstr->PcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);
#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
			phmulcnstr->Insert(GPOS_NEW(memory_pool) ULONG(ul), pcnstrRemapped);
			GPOS_ASSERT(fResult);
		}
	}

	m_pbsDefaultParts->AddRef();
	return GPOS_NEW(memory_pool) CPartConstraint(memory_pool, phmulcnstr, m_pbsDefaultParts, m_fUnbounded, pdrgpdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CPartConstraint::OsPrint
	(
	IOstream &os
	)
	const
{
	os << "Part constraint: (";
	if (m_fUninterpreted)
	{
		os << "uninterpreted)";
		return os;
	}

	for (ULONG ul = 0; ul < m_num_of_part_levels; ul++)
	{
		if (ul > 0)
		{
			os << ", ";
		}
		CConstraint *pcnstr = Pcnstr(ul);
		if (NULL != pcnstr)
		{
			pcnstr->OsPrint(os);
		}
		else
		{
			os << "-";
		}
	}
	
	os << ", default partitions on levels: " << *m_pbsDefaultParts
		<< ", unbounded: " << m_fUnbounded;
	os << ")";
	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::FDisjunctionPossible
//
//	@doc:
//		Check if it is possible to produce a disjunction of the two given part
//		constraints. This is possible if the first ulLevels-1 have the same
//		constraints and default flags for both part constraints
//
//---------------------------------------------------------------------------
BOOL
CPartConstraint::FDisjunctionPossible
	(
	CPartConstraint *ppartcnstrFst,
	CPartConstraint *ppartcnstrSnd
	)
{
	GPOS_ASSERT(NULL != ppartcnstrFst);
	GPOS_ASSERT(NULL != ppartcnstrSnd);
	GPOS_ASSERT(ppartcnstrFst->m_num_of_part_levels == ppartcnstrSnd->m_num_of_part_levels);

	const ULONG ulLevels = ppartcnstrFst->m_num_of_part_levels;
	BOOL fSuccess = true;

	for (ULONG ul = 0; fSuccess && ul < ulLevels - 1; ul++)
	{
		CConstraint *pcnstrFst = ppartcnstrFst->Pcnstr(ul);
		CConstraint *pcnstrSnd = ppartcnstrSnd->Pcnstr(ul);
		fSuccess = (NULL != pcnstrFst &&
					NULL != pcnstrSnd &&
					pcnstrFst->Equals(pcnstrSnd) &&
					ppartcnstrFst->FDefaultPartition(ul) == ppartcnstrSnd->FDefaultPartition(ul));
	}

	// last level constraints cannot be NULL as well
	fSuccess = (fSuccess &&
				NULL != ppartcnstrFst->Pcnstr(ulLevels - 1) &&
				NULL != ppartcnstrSnd->Pcnstr(ulLevels - 1));

	return fSuccess;
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::PpartcnstrDisjunction
//
//	@doc:
//		Construct a disjunction of the two part constraints. We can only
//		construct this disjunction if they differ only on the last level
//
//---------------------------------------------------------------------------
CPartConstraint *
CPartConstraint::PpartcnstrDisjunction
	(
	IMemoryPool *memory_pool,
	CPartConstraint *ppartcnstrFst,
	CPartConstraint *ppartcnstrSnd
	)
{
	GPOS_ASSERT(NULL != ppartcnstrFst);
	GPOS_ASSERT(NULL != ppartcnstrSnd);

	if (ppartcnstrFst->FUnbounded())
	{
		ppartcnstrFst->AddRef();
		return ppartcnstrFst;
	}
	
	if (ppartcnstrSnd->FUnbounded())
	{
		ppartcnstrSnd->AddRef();
		return ppartcnstrSnd;
	}

	if (!FDisjunctionPossible(ppartcnstrFst, ppartcnstrSnd))
	{
		return NULL;
	}

	HMUlCnstr *phmulcnstr = GPOS_NEW(memory_pool) HMUlCnstr(memory_pool);
	CBitSet *pbsCombined = GPOS_NEW(memory_pool) CBitSet(memory_pool);

	const ULONG ulLevels = ppartcnstrFst->m_num_of_part_levels;
	for (ULONG ul = 0; ul < ulLevels-1; ul++)
	{
		CConstraint *pcnstrFst = ppartcnstrFst->Pcnstr(ul);

		pcnstrFst->AddRef();
#ifdef GPOS_DEBUG
		BOOL fResult =
#endif // GPOS_DEBUG
		phmulcnstr->Insert(GPOS_NEW(memory_pool) ULONG(ul), pcnstrFst);
		GPOS_ASSERT(fResult);

		if (ppartcnstrFst->FDefaultPartition(ul))
		{
			pbsCombined->ExchangeSet(ul);
		}
	}

	// create the disjunction between the constraints of the last level
	CConstraint *pcnstrFst = ppartcnstrFst->Pcnstr(ulLevels - 1);
	CConstraint *pcnstrSnd = ppartcnstrSnd->Pcnstr(ulLevels - 1);

	pcnstrFst->AddRef();
	pcnstrSnd->AddRef();
	DrgPcnstr *pdrgpcnstrCombined = GPOS_NEW(memory_pool) DrgPcnstr(memory_pool);
	
	pdrgpcnstrCombined->Append(pcnstrFst);
	pdrgpcnstrCombined->Append(pcnstrSnd);

	CConstraint *pcnstrDisj = CConstraint::PcnstrDisjunction(memory_pool, pdrgpcnstrCombined);
	GPOS_ASSERT(NULL != pcnstrDisj);
#ifdef GPOS_DEBUG
	BOOL fResult =
#endif // GPOS_DEBUG
	phmulcnstr->Insert(GPOS_NEW(memory_pool) ULONG(ulLevels - 1), pcnstrDisj);
	GPOS_ASSERT(fResult);

	if (ppartcnstrFst->FDefaultPartition(ulLevels - 1) || ppartcnstrSnd->FDefaultPartition(ulLevels - 1))
	{
		pbsCombined->ExchangeSet(ulLevels - 1);
	}

	DrgDrgPcr *pdrgpdrgpcr = ppartcnstrFst->Pdrgpdrgpcr();
	pdrgpdrgpcr->AddRef();
	return GPOS_NEW(memory_pool) CPartConstraint(memory_pool, phmulcnstr, pbsCombined, false /*fUnbounded*/, pdrgpdrgpcr);
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::CopyPartConstraints
//
//	@doc:
//		Copy the part constraints to the given destination part constraint map
//
//---------------------------------------------------------------------------
void
CPartConstraint::CopyPartConstraints
	(
	IMemoryPool *memory_pool,
	PartCnstrMap *ppartcnstrmapDest,
	PartCnstrMap *ppartcnstrmapSource
	)
{
	GPOS_ASSERT(NULL != ppartcnstrmapDest);
	GPOS_ASSERT(NULL != ppartcnstrmapSource);

	PartCnstrMapIter pcmi(ppartcnstrmapSource);

	while (pcmi.Advance())
	{
		ULONG ulKey = *(pcmi.Key());
		CPartConstraint *ppartcnstrSource = const_cast<CPartConstraint *>(pcmi.Value());

		CPartConstraint *ppartcnstrDest = ppartcnstrmapDest->Find(&ulKey);
		GPOS_ASSERT_IMP(NULL != ppartcnstrDest, ppartcnstrDest->FEquivalent(ppartcnstrSource));

		if (NULL == ppartcnstrDest)
		{
			ppartcnstrSource->AddRef();

#ifdef GPOS_DEBUG
			BOOL fResult =
#endif // GPOS_DEBUG
				ppartcnstrmapDest->Insert(GPOS_NEW(memory_pool) ULONG(ulKey), ppartcnstrSource);

			GPOS_ASSERT(fResult && "Duplicate part constraints");
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CPartConstraint::PpartcnstrmapCombine
//
//	@doc:
//		Combine the two given part constraint maps and return the result
//
//---------------------------------------------------------------------------
PartCnstrMap *
CPartConstraint::PpartcnstrmapCombine
	(
	IMemoryPool *memory_pool,
	PartCnstrMap *ppartcnstrmapFst,
	PartCnstrMap *ppartcnstrmapSnd
	)
{
	if (NULL == ppartcnstrmapFst && NULL == ppartcnstrmapSnd)
	{
		return NULL;
	}

	if (NULL == ppartcnstrmapFst)
	{
		ppartcnstrmapSnd->AddRef();
		return ppartcnstrmapSnd;
	}

	if (NULL == ppartcnstrmapSnd)
	{
		ppartcnstrmapFst->AddRef();
		return ppartcnstrmapFst;
	}

	GPOS_ASSERT(NULL != ppartcnstrmapFst);
	GPOS_ASSERT(NULL != ppartcnstrmapSnd);

	PartCnstrMap *ppartcnstrmap = GPOS_NEW(memory_pool) PartCnstrMap(memory_pool);

	CopyPartConstraints(memory_pool, ppartcnstrmap, ppartcnstrmapFst);
	CopyPartConstraints(memory_pool, ppartcnstrmap, ppartcnstrmapSnd);

	return ppartcnstrmap;
}

// EOF

