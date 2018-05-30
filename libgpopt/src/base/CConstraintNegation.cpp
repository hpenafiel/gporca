//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintNegation.cpp
//
//	@doc:
//		Implementation of negation constraints
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CConstraintNegation.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::CConstraintNegation
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CConstraintNegation::CConstraintNegation
	(
	IMemoryPool *memory_pool,
	CConstraint *pcnstr
	)
	:
	CConstraint(memory_pool),
	m_pcnstr(pcnstr)
{
	GPOS_ASSERT(NULL != pcnstr);

	m_pcrsUsed = pcnstr->PcrsUsed();
	m_pcrsUsed->AddRef();
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::~CConstraintNegation
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CConstraintNegation::~CConstraintNegation()
{
	m_pcnstr->Release();
	m_pcrsUsed->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::PcnstrCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the constraint with remapped columns
//
//---------------------------------------------------------------------------
CConstraint *
CConstraintNegation::PcnstrCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	CConstraint *pcnstr = m_pcnstr->PcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);
	return GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, pcnstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::Pcnstr
//
//	@doc:
//		Return constraint on a given column
//
//---------------------------------------------------------------------------
CConstraint *
CConstraintNegation::Pcnstr
	(
	IMemoryPool *memory_pool,
	const CColRef *pcr
	)
{
	if (!m_pcrsUsed->FMember(pcr))
	{
		return NULL;
	}

	// donot recurse down the constraint, return the complete constraint
	// on a given column which may include other columns as well
	// in case of NOT (negation) we cannot further simplify the constraint.
	// for instance, conjunction constraint (NOT a=b) is like:
	//	 NOT ({"a" (0), ranges: (-inf, inf) } AND {"b" (1), ranges: (-inf, inf) }))
	// recursing down the constraint will give NOT ({"a" (0), ranges: (-inf, inf) })
	// but that is equivalent to (NOT a) which is not the case.
	m_pcnstr->AddRef();
	return GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, m_pcnstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::Pcnstr
//
//	@doc:
//		Return constraint on a given column set
//
//---------------------------------------------------------------------------
CConstraint *
CConstraintNegation::Pcnstr
	(
	IMemoryPool *memory_pool,
	CColRefSet *pcrs
	)
{
	if (m_pcrsUsed->IsDisjoint(pcrs))
	{
		return NULL;
	}

	return GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, m_pcnstr->Pcnstr(memory_pool, pcrs));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::PcnstrRemapForColumn
//
//	@doc:
//		Return a copy of the constraint for a different column
//
//---------------------------------------------------------------------------
CConstraint *
CConstraintNegation::PcnstrRemapForColumn
	(
	IMemoryPool *memory_pool,
	CColRef *pcr
	)
	const
{
	GPOS_ASSERT(1 == m_pcrsUsed->Size());

	return GPOS_NEW(memory_pool) CConstraintNegation(memory_pool, m_pcnstr->PcnstrRemapForColumn(memory_pool, pcr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::PexprScalar
//
//	@doc:
//		Scalar expression
//
//---------------------------------------------------------------------------
CExpression *
CConstraintNegation::PexprScalar
	(
	IMemoryPool *memory_pool
	)
{
	if (NULL == m_pexprScalar)
	{
		EConstraintType ect = m_pcnstr->Ect();
		if (EctNegation == ect)
		{
			CConstraintNegation *pcn = (CConstraintNegation *)m_pcnstr;
			m_pexprScalar = pcn->PcnstrChild()->PexprScalar(memory_pool);
			m_pexprScalar->AddRef();
		}
		else if (EctInterval == ect)
		{
			CConstraintInterval *pci = (CConstraintInterval *)m_pcnstr;
			CConstraintInterval *pciComp = pci->PciComplement(memory_pool);
			m_pexprScalar = pciComp->PexprScalar(memory_pool);
			m_pexprScalar->AddRef();
			pciComp->Release();
		}
		else
		{
			CExpression *pexpr = m_pcnstr->PexprScalar(memory_pool);
			pexpr->AddRef();
			m_pexprScalar = CUtils::PexprNegate(memory_pool, pexpr);
		}
	}

	return m_pexprScalar;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CConstraintNegation::OsPrint
	(
	IOstream &os
	)
	const
{
	os << "(NOT " << *m_pcnstr << ")";

	return os;
}

// EOF
