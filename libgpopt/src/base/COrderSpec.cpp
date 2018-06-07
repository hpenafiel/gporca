//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		COrderSpec.cpp
//
//	@doc:
//		Specification of order property
//---------------------------------------------------------------------------

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CPhysicalSort.h"

#ifdef GPOS_DEBUG
#include "gpos/error/CAutoTrace.h"
#endif // GPOS_DEBUG

using namespace gpopt;
using namespace gpmd;

// string encoding of null treatment
const CHAR rgszNullCode[][16] = {"Auto", "NULLsFirst", "NULLsLast"};
GPOS_CPL_ASSERT(COrderSpec::EntSentinel == GPOS_ARRAY_SIZE(rgszNullCode));


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::COrderExpression::COrderExpression
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
COrderSpec::COrderExpression::COrderExpression
	(
	gpmd::IMDId *pmdid,
	const CColRef *pcr,
	ENullTreatment ent
	)
	:
	m_mdid(pmdid),
	m_pcr(pcr),
	m_ent(ent)
{
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(pmdid->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::COrderExpression::~COrderExpression
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
COrderSpec::COrderExpression::~COrderExpression()
{
	m_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::COrderExpression::FMatch
//
//	@doc:
//		Check if order expression equal to given one;
//
//---------------------------------------------------------------------------
BOOL
COrderSpec::COrderExpression::FMatch
	(
	const COrderExpression *poe
	)
	const
{
	GPOS_ASSERT(NULL != poe);
		
	return
		poe->m_mdid->Equals(m_mdid) && 
		poe->m_pcr == m_pcr &&
		poe->m_ent == m_ent;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::COrderExpression::OsPrint
//
//	@doc:
//		Print order expression
//
//---------------------------------------------------------------------------
IOstream &
COrderSpec::COrderExpression::OsPrint
	(
	IOstream &os
	)
	const
{
	os << "( ";
	m_mdid->OsPrint(os);
	os << ", ";
	m_pcr->OsPrint(os);
	os << ", " << rgszNullCode[m_ent] << " )";

	return os;
}

#ifdef GPOS_DEBUG
void
COrderSpec::COrderExpression::DbgPrint() const
{
	IMemoryPool *memory_pool = COptCtxt::PoctxtFromTLS()->Pmp();
	CAutoTrace at(memory_pool);
	(void) this->OsPrint(at.Os());
}
#endif // GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::COrderSpec
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
COrderSpec::COrderSpec
	(
	IMemoryPool *memory_pool
	)
	:
	m_memory_pool(memory_pool),
	m_pdrgpoe(NULL)
{
	m_pdrgpoe = GPOS_NEW(memory_pool) DrgPoe(memory_pool);
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::~COrderSpec
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
COrderSpec::~COrderSpec()
{
	m_pdrgpoe->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::Append
//
//	@doc:
//		Append order expression;
//
//---------------------------------------------------------------------------
void
COrderSpec::Append
	(
	gpmd::IMDId *pmdid,
	const CColRef *pcr,
	ENullTreatment ent
	)
{
	COrderExpression *poe = GPOS_NEW(m_memory_pool) COrderExpression(pmdid, pcr, ent);
	m_pdrgpoe->Append(poe);
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::FMatch
//
//	@doc:
//		Check for equality between order specs
//
//---------------------------------------------------------------------------
BOOL
COrderSpec::FMatch
	(
	const COrderSpec *pos
	)
	const
{
	BOOL fMatch = 
			m_pdrgpoe->Size() == pos->m_pdrgpoe->Size() && 
			FSatisfies(pos);
		
	GPOS_ASSERT_IMP(fMatch, pos->FSatisfies(this));
		
	return fMatch;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::FSatisfies
//
//	@doc:
//		Check if this order spec satisfies the given one
//
//---------------------------------------------------------------------------
BOOL
COrderSpec::FSatisfies
	(
	const COrderSpec *pos
	)
	const
{	
	const ULONG arity = pos->m_pdrgpoe->Size();
	BOOL fSatisfies = (m_pdrgpoe->Size() >= arity);
	
	for (ULONG ul = 0; fSatisfies && ul < arity; ul++)
	{
		fSatisfies = (*m_pdrgpoe)[ul]->FMatch((*(pos->m_pdrgpoe))[ul]);
	}
	
	return fSatisfies;	
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::AppendEnforcers
//
//	@doc:
//		Add required enforcers enforcers to dynamic array
//
//---------------------------------------------------------------------------
void
COrderSpec::AppendEnforcers
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &, // exprhdl
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
	GPOS_ASSERT(this == prpp->Peo()->PosRequired() &&
				"required plan properties don't match enforced order spec");

	AddRef();
	pexpr->AddRef();
	CExpression *pexprSort = GPOS_NEW(memory_pool) CExpression
										(
										memory_pool, 
										GPOS_NEW(memory_pool) CPhysicalSort(memory_pool, this),
										pexpr
										);
	pdrgpexpr->Append(pexprSort);
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::HashValue
//
//	@doc:
//		Hash of components
//
//---------------------------------------------------------------------------
ULONG
COrderSpec::HashValue() const
{
	ULONG ulHash = 0;
	ULONG arity = m_pdrgpoe->Size();
	
	for (ULONG ul = 0; ul < arity; ul++)
	{
		COrderExpression *poe = (*m_pdrgpoe)[ul];
		ulHash = gpos::CombineHashes(ulHash, gpos::HashPtr<CColRef>(poe->Pcr()));
	}
	
	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::PosCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the order spec with remapped columns
//
//---------------------------------------------------------------------------
COrderSpec *
COrderSpec::PosCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	COrderSpec *pos = GPOS_NEW(memory_pool) COrderSpec(memory_pool);

	const ULONG ulCols = m_pdrgpoe->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		COrderExpression *poe = (*m_pdrgpoe)[ul];
		IMDId *pmdid = poe->GetMdIdSortOp();
		pmdid->AddRef();

		const CColRef *pcr = poe->Pcr();
		ULONG id = pcr->Id();
		CColRef *pcrMapped = phmulcr->Find(&id);
		if (NULL == pcrMapped)
		{
			if (fMustExist)
			{
				CColumnFactory *pcf = COptCtxt::PoctxtFromTLS()->Pcf();
				// not found in hashmap, so create a new colref and add to hashmap
				pcrMapped = pcf->PcrCopy(pcr);

#ifdef GPOS_DEBUG
				BOOL fResult =
#endif // GPOS_DEBUG
				phmulcr->Insert(GPOS_NEW(memory_pool) ULONG(id), pcrMapped);
				GPOS_ASSERT(fResult);
			}
			else
			{
				pcrMapped = const_cast<CColRef*>(pcr);
			}
		}

		COrderSpec::ENullTreatment ent = poe->Ent();
		pos->Append(pmdid, pcrMapped, ent);
	}

	return pos;
}

//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::PosExcludeColumns
//
//	@doc:
//		Return a copy of the order spec after excluding the given columns
//
//---------------------------------------------------------------------------
COrderSpec *
COrderSpec::PosExcludeColumns
	(
	IMemoryPool *memory_pool,
	CColRefSet *pcrs
	)
{
	GPOS_ASSERT(NULL != pcrs);

	COrderSpec *pos = GPOS_NEW(memory_pool) COrderSpec(memory_pool);

	const ULONG ulCols = m_pdrgpoe->Size();
	for (ULONG ul = 0; ul < ulCols; ul++)
	{
		COrderExpression *poe = (*m_pdrgpoe)[ul];
		const CColRef *pcr = poe->Pcr();

		if (pcrs->FMember(pcr))
		{
			continue;
		}

		IMDId *pmdid = poe->GetMdIdSortOp();
		pmdid->AddRef();
		pos->Append(pmdid, pcr, poe->Ent());
	}

	return pos;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::ExtractCols
//
//	@doc:
//		Extract columns from order spec into the given column set
//
//---------------------------------------------------------------------------
void
COrderSpec::ExtractCols
	(
	CColRefSet *pcrs
	)
	const
{
	GPOS_ASSERT(NULL != pcrs);

	const ULONG ulOrderExprs = m_pdrgpoe->Size();
	for (ULONG ul = 0; ul < ulOrderExprs; ul++)
	{
		pcrs->Include((*m_pdrgpoe)[ul]->Pcr());
	}
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::PcrsUsed
//
//	@doc:
//		Extract colref set from order components
//
//---------------------------------------------------------------------------
CColRefSet *
COrderSpec::PcrsUsed
	(
	IMemoryPool *memory_pool
	)
	const
{
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	ExtractCols(pcrs);
	
	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::Pcrs
//
//	@doc:
//		Extract colref set from order specs in the given array
//
//---------------------------------------------------------------------------
CColRefSet *
COrderSpec::Pcrs
	(
	IMemoryPool *memory_pool,
	DrgPos *pdrgpos
	)
{
	GPOS_ASSERT(NULL != pdrgpos);

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	const ULONG ulOrderSpecs = pdrgpos->Size();
	for (ULONG ulSpec = 0; ulSpec < ulOrderSpecs; ulSpec++)
	{
		COrderSpec *pos = (*pdrgpos)[ulSpec];
		pos->ExtractCols(pcrs);
	}

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::PdrgposExclude
//
//	@doc:
//		Filter out array of order specs from order expressions using the
//		passed columns
//
//---------------------------------------------------------------------------
DrgPos *
COrderSpec::PdrgposExclude
	(
	IMemoryPool *memory_pool,
	DrgPos *pdrgpos,
	CColRefSet *pcrsToExclude
	)
{
	GPOS_ASSERT(NULL != pdrgpos);
	GPOS_ASSERT(NULL != pcrsToExclude);

	if (0 == pcrsToExclude->Size())
	{
		// no columns to exclude
		pdrgpos->AddRef();
		return pdrgpos;
	}

	DrgPos *pdrgposNew = GPOS_NEW(memory_pool) DrgPos(memory_pool);
	const ULONG ulOrderSpecs = pdrgpos->Size();
	for (ULONG ulSpec = 0; ulSpec < ulOrderSpecs; ulSpec++)
	{
		COrderSpec *pos = (*pdrgpos)[ulSpec];
		COrderSpec *posNew = pos->PosExcludeColumns(memory_pool, pcrsToExclude);
		pdrgposNew->Append(posNew);
	}

	return pdrgposNew;
}

//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::OsPrint
//
//	@doc:
//		Print order spec
//
//---------------------------------------------------------------------------
IOstream &
COrderSpec::OsPrint
	(
	IOstream &os
	)
	const
{
	const ULONG arity = m_pdrgpoe->Size();
	if (0 == arity)
	{
		os << "<empty>";
	}
	else 
	{
		for (ULONG ul = 0; ul < arity; ul++)
		{
			(*m_pdrgpoe)[ul]->OsPrint(os) << " ";
		}
	}

	return os;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::Equals
//
//	@doc:
//		 Matching function over order spec arrays
//
//---------------------------------------------------------------------------
BOOL
COrderSpec::Equals
	(
	const DrgPos *pdrgposFirst,
	const DrgPos *pdrgposSecond
	)
{
	if (NULL == pdrgposFirst || NULL == pdrgposSecond)
	{
		return (NULL == pdrgposFirst && NULL ==pdrgposSecond);
	}

	if (pdrgposFirst->Size() != pdrgposSecond->Size())
	{
		return false;
	}

	const ULONG size = pdrgposFirst->Size();
	BOOL fMatch = true;
	for (ULONG ul = 0; fMatch && ul < size; ul++)
	{
		fMatch = (*pdrgposFirst)[ul]->FMatch((*pdrgposSecond)[ul]);
	}

	return fMatch;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::HashValue
//
//	@doc:
//		 Combine hash values of a maximum number of entries
//
//---------------------------------------------------------------------------
ULONG
COrderSpec::HashValue
	(
	const DrgPos *pdrgpos,
	ULONG ulMaxSize
	)
{
	GPOS_ASSERT(NULL != pdrgpos);
	ULONG size = std::min(ulMaxSize, pdrgpos->Size());

	ULONG ulHash = 0;
	for (ULONG ul = 0; ul < size; ul++)
	{
		ulHash = gpos::CombineHashes(ulHash, (*pdrgpos)[ul]->HashValue());
	}

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderSpec::OsPrint
//
//	@doc:
//		 Print array of order spec objects
//
//---------------------------------------------------------------------------
IOstream &
COrderSpec::OsPrint
	(
	IOstream &os,
	const DrgPos *pdrgpos
	)
{
	const ULONG size = pdrgpos->Size();
	os	<< "[";
	if (0 < size)
	{
		for (ULONG ul = 0; ul < size - 1; ul++)
		{
			(void) (*pdrgpos)[ul]->OsPrint(os);
			os <<	", ";
		}

		(void) (*pdrgpos)[size - 1]->OsPrint(os);
	}

	return os << "]";
}


// EOF

