//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDynamicGet.cpp
//
//	@doc:
//		Implementation of dynamic table access
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CPartIndexMap.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalDynamicGet.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/metadata/CName.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::CLogicalDynamicGet
//
//	@doc:
//		ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::CLogicalDynamicGet
	(
	IMemoryPool *memory_pool
	)
	:
	CLogicalDynamicGetBase(memory_pool)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::CLogicalDynamicGet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::CLogicalDynamicGet
	(
	IMemoryPool *memory_pool,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc,
	ULONG ulPartIndex,
	DrgPcr *pdrgpcrOutput,
	DrgDrgPcr *pdrgpdrgpcrPart,
	ULONG ulSecondaryPartIndexId,
	BOOL fPartial,
	CPartConstraint *ppartcnstr, 
	CPartConstraint *ppartcnstrRel
	)
	:
	CLogicalDynamicGetBase(memory_pool, pnameAlias, ptabdesc, ulPartIndex, pdrgpcrOutput, pdrgpdrgpcrPart, ulSecondaryPartIndexId, fPartial, ppartcnstr, ppartcnstrRel)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::CLogicalDynamicGet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::CLogicalDynamicGet
	(
	IMemoryPool *memory_pool,
	const CName *pnameAlias,
	CTableDescriptor *ptabdesc,
	ULONG ulPartIndex
	)
	:
	CLogicalDynamicGetBase(memory_pool, pnameAlias, ptabdesc, ulPartIndex)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::~CLogicalDynamicGet
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::~CLogicalDynamicGet()
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDynamicGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(), m_ptabdesc->MDId()->HashValue());
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicGet::FMatch
	(
	COperator *pop
	)
	const
{
	return CUtils::FMatchDynamicScan(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalDynamicGet::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	DrgPcr *pdrgpcrOutput = NULL;
	if (fMustExist)
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemapAndCreate(memory_pool, m_pdrgpcrOutput, phmulcr);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(memory_pool, m_pdrgpcrOutput, phmulcr, fMustExist);
	}
	DrgDrgPcr *pdrgpdrgpcrPart = PdrgpdrgpcrCreatePartCols(memory_pool, pdrgpcrOutput, m_ptabdesc->PdrgpulPart());
	CName *pnameAlias = GPOS_NEW(memory_pool) CName(memory_pool, *m_pnameAlias);
	m_ptabdesc->AddRef();

	CPartConstraint *ppartcnstr = m_part_constraint->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);
	CPartConstraint *ppartcnstrRel = m_ppartcnstrRel->PpartcnstrCopyWithRemappedColumns(memory_pool, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CLogicalDynamicGet(memory_pool, pnameAlias, m_ptabdesc, m_scan_id, pdrgpcrOutput, pdrgpdrgpcrPart, m_ulSecondaryScanId, m_fPartial, ppartcnstr, ppartcnstrRel);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::FInputOrderSensitive
//
//	@doc:
//		Not called for leaf operators
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicGet::FInputOrderSensitive() const
{
	GPOS_ASSERT(!"Unexpected function call of FInputOrderSensitive");
	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalDynamicGet::PxfsCandidates
	(
	IMemoryPool *memory_pool
	) 
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfDynamicGet2DynamicTableScan);
	return pxfs;
}



//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDynamicGet::OsPrint
	(
	IOstream &os
	)
	const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}
	else
	{
		os << SzId() << " ";
		// alias of table as referenced in the query
		m_pnameAlias->OsPrint(os);

		// actual name of table in catalog and columns
		os << " (";
		m_ptabdesc->Name().OsPrint(os);
		os <<"), ";
		m_part_constraint->OsPrint(os);
		os << "), Columns: [";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
		os << "] Scan Id: " << m_scan_id << "." << m_ulSecondaryScanId;
		
		if (!m_part_constraint->FUnbounded())
		{
			os << ", ";
			m_part_constraint->OsPrint(os);
		}
	}
		
	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PstatsDerive
//
//	@doc:
//		Load up statistics from metadata
//
//---------------------------------------------------------------------------
IStatistics *
CLogicalDynamicGet::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // not used
	)
	const
{
	CReqdPropRelational *prprel = CReqdPropRelational::Prprel(exprhdl.Prp());
	IStatistics *pstats = PstatsDeriveFilter(memory_pool, exprhdl, prprel->PexprPartPred());

	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool, m_pdrgpcrOutput);
	CUpperBoundNDVs *pubndv = GPOS_NEW(memory_pool) CUpperBoundNDVs(pcrs, pstats->Rows());
	CStatistics::PstatsConvert(pstats)->AddCardUpperBound(pubndv);

	return pstats;
}

// EOF

