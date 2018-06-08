//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CRange.cpp
//
//	@doc:
//		Implementation of ranges
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CRange.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/IComparator.h"
#include "gpopt/operators/CPredicateUtils.h"

#include "naucrates/md/IMDScalarOp.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CRange::CRange
//
//	@doc:
//		Ctor
//		Does not take ownership of 'pcomp'.
//
//---------------------------------------------------------------------------
CRange::CRange
	(
	IMDId *mdid,
	const IComparator *pcomp,
	IDatum *pdatumLeft,
	ERangeInclusion eriLeft,
	IDatum *pdatumRight,
	ERangeInclusion eriRight
	)
	:
	m_mdid(mdid),
	m_pcomp(pcomp),
	m_pdatumLeft(pdatumLeft),
	m_eriLeft(eriLeft),
	m_pdatumRight(pdatumRight),
	m_eriRight(eriRight)
{
	GPOS_ASSERT(mdid->IsValid());
	GPOS_ASSERT(NULL != pcomp);
	GPOS_ASSERT(CUtils::FConstrainableType(mdid));
	GPOS_ASSERT_IMP(NULL != pdatumLeft && NULL != pdatumRight,
			pcomp->FLessThanOrEqual(pdatumLeft, pdatumRight));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::CRange
//
//	@doc:
//		Ctor
//		Does not take ownership of 'pcomp'.
//
//---------------------------------------------------------------------------
CRange::CRange
	(
	const IComparator *pcomp,
	IMDType::ECmpType ecmpt,
	IDatum *pdatum
	)
	:
	m_mdid(NULL),
	m_pcomp(pcomp),
	m_pdatumLeft(NULL),
	m_eriLeft(EriExcluded),
	m_pdatumRight(NULL),
	m_eriRight(EriExcluded)
{
	m_mdid = pdatum->MDId();

	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(NULL != pcomp);
	GPOS_ASSERT(CUtils::FConstrainableType(m_mdid));
	m_mdid->AddRef();

	switch (ecmpt)
	{
		case IMDType::EcmptEq:
		{
			pdatum->AddRef();
			m_pdatumLeft = pdatum;
			m_pdatumRight = pdatum;
			m_eriLeft = EriIncluded;
			m_eriRight = EriIncluded;
			break;
		}

		case IMDType::EcmptL:
		{
			m_pdatumRight = pdatum;
			break;
		}

		case IMDType::EcmptLEq:
		{
			m_pdatumRight = pdatum;
			m_eriRight = EriIncluded;
			break;
		}

		case IMDType::EcmptG:
		{
			m_pdatumLeft = pdatum;
			break;
		}

		case IMDType::EcmptGEq:
		{
			m_pdatumLeft = pdatum;
			m_eriLeft = EriIncluded;
			break;
		}

		default:
			// for anything else, create a (-inf, inf) range
			break;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::~CRange
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CRange::~CRange()
{
	m_mdid->Release();
	CRefCount::SafeRelease(m_pdatumLeft);
	CRefCount::SafeRelease(m_pdatumRight);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FDisjointLeft
//
//	@doc:
//		Is this range disjoint from the given range and to its left
//
//---------------------------------------------------------------------------
BOOL
CRange::FDisjointLeft
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	IDatum *pdatumLeft = prange->PdatumLeft();

	if (NULL == m_pdatumRight || NULL == pdatumLeft)
	{
		return false;
	}

	if (m_pcomp->FLessThan(m_pdatumRight, pdatumLeft))
	{
		return true;
	}

	if (m_pcomp->Equals(m_pdatumRight, pdatumLeft))
	{
		return (EriExcluded == m_eriRight || EriExcluded == prange->EriLeft());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FContains
//
//	@doc:
//		Does this range contain the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FContains
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	return FStartsWithOrBefore(prange) && FEndsWithOrAfter(prange);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FOverlapsLeft
//
//	@doc:
//		Does this range overlap only the left end of the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FOverlapsLeft
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	return (FStartsBefore(prange) &&
			!FEndsAfter(prange) &&
			!FDisjointLeft(prange));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FOverlapsRight
//
//	@doc:
//		Does this range overlap only the right end of the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FOverlapsRight
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	return (FEndsAfter(prange) &&
			!FStartsBefore(prange) &&
			!prange->FDisjointLeft(this));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FUpperBoundEqualsLowerBound
//
//	@doc:
//		Checks if this range's upper bound value is equal to the given range's
//		lower bound value. Ignores inclusivity/exclusivity Examples:
//			(-inf, 8)(8, inf)	true
//			(-inf, 8](8, inf)	true
//			(-inf, inf)(8, inf)	false
//
//---------------------------------------------------------------------------
BOOL
CRange::FUpperBoundEqualsLowerBound
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	IDatum *pdatumLeft = prange->PdatumLeft();

	if (NULL == pdatumLeft && NULL == m_pdatumRight)
	{
		return true;
	}

	if (NULL == pdatumLeft || NULL == m_pdatumRight)
	{
		return false;
	}

	return m_pcomp->Equals(m_pdatumRight, pdatumLeft);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FStartsWithOrBefore
//
//	@doc:
//		Does this range start with or before the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FStartsWithOrBefore
	(
	CRange *prange
	)
{
	if (FStartsBefore(prange))
	{
		return true;
	}

	IDatum *pdatumLeft = prange->PdatumLeft();
	if (NULL == pdatumLeft && NULL == m_pdatumLeft)
	{
		return true;
	}

	if (NULL == pdatumLeft || NULL == m_pdatumLeft)
	{
		return false;
	}

	return (m_pcomp->Equals(m_pdatumLeft, pdatumLeft) && m_eriLeft == prange->EriLeft());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FStartsBefore
//
//	@doc:
//		Does this range start before the given range starts
//
//---------------------------------------------------------------------------
BOOL
CRange::FStartsBefore
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	IDatum *pdatumLeft = prange->PdatumLeft();
	if (NULL == pdatumLeft)
	{
		return (NULL == m_pdatumLeft);
	}

	if (NULL == m_pdatumLeft || m_pcomp->FLessThan(m_pdatumLeft, pdatumLeft))
	{
		return true;
	}

	if (m_pcomp->FGreaterThan(m_pdatumLeft, pdatumLeft))
	{
		return false;
	}

	GPOS_ASSERT(m_pcomp->Equals(m_pdatumLeft, pdatumLeft));

	return (EriIncluded == m_eriLeft && EriExcluded == prange->EriLeft());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FEndsAfter
//
//	@doc:
//		Does this range end after the given range ends
//
//---------------------------------------------------------------------------
BOOL
CRange::FEndsAfter
	(
	CRange *prange
	)
{
	GPOS_ASSERT(NULL != prange);

	IDatum *pdatumRight = prange->PdatumRight();
	if (NULL == pdatumRight)
	{
		return (NULL == m_pdatumRight);
	}

	if (NULL == m_pdatumRight || m_pcomp->FGreaterThan(m_pdatumRight, pdatumRight))
	{
		return true;
	}

	if (m_pcomp->FLessThan(m_pdatumRight, pdatumRight))
	{
		return false;
	}

	GPOS_ASSERT(m_pcomp->Equals(m_pdatumRight, pdatumRight));

	return (EriIncluded == m_eriRight && EriExcluded == prange->EriRight());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FEndsWithOrAfter
//
//	@doc:
//		Does this range end with or after the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FEndsWithOrAfter
	(
	CRange *prange
	)
{
	if (FEndsAfter(prange))
	{
		return true;
	}

	IDatum *pdatumRight = prange->PdatumRight();
	if (NULL == pdatumRight && NULL == m_pdatumRight)
	{
		return true;
	}

	if (NULL == pdatumRight || NULL == m_pdatumRight)
	{
		return false;
	}

	return (m_pcomp->Equals(m_pdatumRight, pdatumRight) && m_eriRight == prange->EriRight());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FPoint
//
//	@doc:
//		Is the range a point
//
//---------------------------------------------------------------------------
BOOL
CRange::FPoint() const
{
	return (EriIncluded == m_eriLeft && EriIncluded == m_eriRight &&
			m_pcomp->Equals(m_pdatumRight, m_pdatumLeft));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PexprScalar
//
//	@doc:
//		Construct scalar comparison expression using given column
//
//---------------------------------------------------------------------------
CExpression *
CRange::PexprScalar
	(
	IMemoryPool *memory_pool,
	const CColRef *pcr
	)
{
	CExpression *pexprEq = PexprEquality(memory_pool, pcr);
	if (NULL != pexprEq)
	{
		return pexprEq;
	}

	CExpression *pexprLeft = PexprScalarCompEnd
								(
								memory_pool,
								m_pdatumLeft,
								m_eriLeft,
								IMDType::EcmptGEq,
								IMDType::EcmptG,
								pcr
								);

	CExpression *pexprRight = PexprScalarCompEnd
								(
								memory_pool,
								m_pdatumRight,
								m_eriRight,
								IMDType::EcmptLEq,
								IMDType::EcmptL,
								pcr
								);

	DrgPexpr *pdrgpexpr = GPOS_NEW(memory_pool) DrgPexpr(memory_pool);

	if (NULL != pexprLeft)
	{
		pdrgpexpr->Append(pexprLeft);
	}

	if (NULL != pexprRight)
	{
		pdrgpexpr->Append(pexprRight);
	}

	return CPredicateUtils::PexprConjunction(memory_pool, pdrgpexpr);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PexprEquality
//
//	@doc:
//		Construct an equality predicate if possible
//
//---------------------------------------------------------------------------
CExpression *
CRange::PexprEquality
	(
	IMemoryPool *memory_pool,
	const CColRef *pcr
	)
{
	if (NULL == m_pdatumLeft || NULL == m_pdatumRight ||
		!m_pcomp->Equals(m_pdatumLeft, m_pdatumRight) || EriExcluded == m_eriLeft || EriExcluded == m_eriRight)
	{
		// not an equality predicate
		return NULL;
	}

	m_pdatumLeft->AddRef();
	CExpression *pexprVal = GPOS_NEW(memory_pool) CExpression(memory_pool, GPOS_NEW(memory_pool) CScalarConst(memory_pool, m_pdatumLeft));

	return CUtils::PexprScalarCmp(memory_pool, pcr, pexprVal, IMDType::EcmptEq);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PexprScalarCompEnd
//
//	@doc:
//		Construct a scalar comparison expression from one of the ends
//
//---------------------------------------------------------------------------
CExpression *
CRange::PexprScalarCompEnd
	(
	IMemoryPool *memory_pool,
	IDatum *pdatum,
	ERangeInclusion eri,
	IMDType::ECmpType ecmptIncl,
	IMDType::ECmpType ecmptExcl,
	const CColRef *pcr
	)
{
	if (NULL == pdatum)
	{
		// unbounded end
		return NULL;
	}

	pdatum->AddRef();
	CExpression *pexprVal = GPOS_NEW(memory_pool) CExpression(memory_pool, GPOS_NEW(memory_pool) CScalarConst(memory_pool, pdatum));

	IMDType::ECmpType ecmpt;
	if (EriIncluded == eri)
	{
		ecmpt = ecmptIncl;
	}
	else
	{
		ecmpt = ecmptExcl;
	}

	return CUtils::PexprScalarCmp(memory_pool, pcr, pexprVal, ecmpt);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngIntersect
//
//	@doc:
//		Intersection with another range
//
//---------------------------------------------------------------------------
CRange *
CRange::PrngIntersect
	(
	IMemoryPool *memory_pool,
	CRange *prange
	)
{
	if (FContains(prange))
	{
		prange->AddRef();
		return prange;
	}

	if (prange->FContains(this))
	{
		this->AddRef();
		return this;
	}

	if (FOverlapsLeft(prange))
	{
		m_mdid->AddRef();

		IDatum *pdatumLeft = prange->PdatumLeft();
		pdatumLeft->AddRef();
		m_pdatumRight->AddRef();

		return GPOS_NEW(memory_pool) CRange(m_mdid, m_pcomp, pdatumLeft, prange->EriLeft(), m_pdatumRight, m_eriRight);
	}

	if (FOverlapsRight(prange))
	{
		m_mdid->AddRef();

		IDatum *pdatumRight = prange->PdatumRight();
		pdatumRight->AddRef();
		m_pdatumLeft->AddRef();

		return GPOS_NEW(memory_pool) CRange(m_mdid, m_pcomp, m_pdatumLeft, m_eriLeft, pdatumRight, prange->EriRight());
	}

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngDifferenceLeft
//
//	@doc:
//		Difference between this range and a given range on the left side only
//
//		this    |----------------------|
//		prange         |-----------|
//		result  |------|
//---------------------------------------------------------------------------
CRange *
CRange::PrngDifferenceLeft
	(
	IMemoryPool *memory_pool,
	CRange *prange
	)
{
	if (FDisjointLeft(prange))
	{
		this->AddRef();
		return this;
	}

	if (FStartsBefore(prange) && NULL != prange->PdatumLeft())
	{
		m_mdid->AddRef();

		if (NULL != m_pdatumLeft)
		{
			m_pdatumLeft->AddRef();
		}

		IDatum *pdatumRight = prange->PdatumLeft();
		pdatumRight->AddRef();

		return GPOS_NEW(memory_pool) CRange
							(
							m_mdid,
							m_pcomp,
							m_pdatumLeft,
							m_eriLeft,
							pdatumRight,
							EriInverseInclusion(prange->EriLeft())
							);
	}

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngDifferenceRight
//
//	@doc:
//		Difference between this range and a given range on the right side only
//
//		this    |----------------------|
//		prange      |-----------|
//		result                  |------|
//---------------------------------------------------------------------------
CRange *
CRange::PrngDifferenceRight
	(
	IMemoryPool *memory_pool,
	CRange *prange
	)
{
	if (prange->FDisjointLeft(this))
	{
		this->AddRef();
		return this;
	}

	if (FEndsAfter(prange) && NULL != prange->PdatumRight())
	{
		m_mdid->AddRef();

		if (NULL != m_pdatumRight)
		{
			m_pdatumRight->AddRef();
		}

		IDatum *pdatumRight = prange->PdatumRight();
		pdatumRight->AddRef();

		return GPOS_NEW(memory_pool) CRange
							(
							m_mdid,
							m_pcomp,
							pdatumRight,
							EriInverseInclusion(prange->EriRight()),
							m_pdatumRight,
							m_eriRight
							);
	}

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngExtend
//
//	@doc:
//		Return the extension of this range with the given range. The given range
//		must start right after this range, otherwise NULL is returned
//
//---------------------------------------------------------------------------
CRange *
CRange::PrngExtend
	(
	IMemoryPool *memory_pool,
	CRange *prange
	)
{
	if (FDisjointLeft(prange) &&
		m_pcomp->Equals(prange->PdatumLeft(), m_pdatumRight) &&
		(EriIncluded == prange->EriLeft() || EriIncluded == m_eriRight))
	{
		// ranges are contiguous so combine them into one
		m_mdid->AddRef();

		if (NULL != m_pdatumLeft)
		{
			m_pdatumLeft->AddRef();
		}

		IDatum *pdatumRight = prange->PdatumRight();
		if (NULL != pdatumRight)
		{
			pdatumRight->AddRef();
		}

		return GPOS_NEW(memory_pool) CRange(m_mdid, m_pcomp, m_pdatumLeft, m_eriLeft, pdatumRight, prange->EriRight());
	}

	return NULL;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CRange::OsPrint
	(
	IOstream &os
	)
	const
{
	if (EriIncluded == m_eriLeft)
	{
		os << "[";
	}
	else
	{
		os << "(";
	}

	OsPrintBound(os, m_pdatumLeft, "-inf");
	os << ", ";
	OsPrintBound(os, m_pdatumRight, "inf");

	if (EriIncluded == m_eriRight)
	{
		os << "]";
	}
	else
	{
		os << ")";
	}

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::OsPrintPoint
//
//	@doc:
//		debug print a point
//
//---------------------------------------------------------------------------
IOstream &
CRange::OsPrintBound
	(
	IOstream &os,
	IDatum *pdatum,
	const CHAR *szInfinity
	)
	const
{
	if (NULL == pdatum)
	{
		os << szInfinity;
	}
	else
	{
		pdatum->OsPrint(os);
	}

	return os;
}

void
CRange::DbgPrint() const
{
	IMemoryPool *memory_pool = COptCtxt::PoctxtFromTLS()->Pmp();
	CAutoTrace at(memory_pool);
	(void) this->OsPrint(at.Os());
}

// EOF
