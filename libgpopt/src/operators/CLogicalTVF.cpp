//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp
//
//	@filename:
//		CLogicalTVF.cpp
//
//	@doc:
//		Implementation of table functions
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/base/CUtils.h"

#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalTVF.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COptCtxt.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::CLogicalTVF
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalTVF::CLogicalTVF
	(
	IMemoryPool *memory_pool
	)
	:
	CLogical(memory_pool),
	m_func_mdid(NULL),
	m_return_type_mdid(NULL),
	m_pstr(NULL),
	m_pdrgpcoldesc(NULL),
	m_pdrgpcrOutput(NULL),
	m_efs(IMDFunction::EfsImmutable),
	m_efda(IMDFunction::EfdaNoSQL),
	m_fReturnsSet(true)
{
	m_fPattern = true;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::CLogicalTVF
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalTVF::CLogicalTVF
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_func,
	IMDId *mdid_return_type,
	CWStringConst *str,
	DrgPcoldesc *pdrgpcoldesc
	)
	:
	CLogical(memory_pool),
	m_func_mdid(mdid_func),
	m_return_type_mdid(mdid_return_type),
	m_pstr(str),
	m_pdrgpcoldesc(pdrgpcoldesc),
	m_pdrgpcrOutput(NULL)
{
	GPOS_ASSERT(mdid_func->IsValid());
	GPOS_ASSERT(mdid_return_type->IsValid());
	GPOS_ASSERT(NULL != str);
	GPOS_ASSERT(NULL != pdrgpcoldesc);

	// generate a default column set for the list of column descriptors
	m_pdrgpcrOutput = PdrgpcrCreateMapping(memory_pool, pdrgpcoldesc, UlOpId());

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDFunction *pmdfunc = md_accessor->Pmdfunc(m_func_mdid);

	m_efs = pmdfunc->EfsStability();
	m_efda = pmdfunc->EfdaDataAccess();
	m_fReturnsSet = pmdfunc->FReturnsSet();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::CLogicalTVF
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalTVF::CLogicalTVF
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_func,
	IMDId *mdid_return_type,
	CWStringConst *str,
	DrgPcoldesc *pdrgpcoldesc,
	DrgPcr *pdrgpcrOutput
	)
	:
	CLogical(memory_pool),
	m_func_mdid(mdid_func),
	m_return_type_mdid(mdid_return_type),
	m_pstr(str),
	m_pdrgpcoldesc(pdrgpcoldesc),
	m_pdrgpcrOutput(pdrgpcrOutput)
{
	GPOS_ASSERT(mdid_func->IsValid());
	GPOS_ASSERT(mdid_return_type->IsValid());
	GPOS_ASSERT(NULL != str);
	GPOS_ASSERT(NULL != pdrgpcoldesc);
	GPOS_ASSERT(NULL != pdrgpcrOutput);

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDFunction *pmdfunc = md_accessor->Pmdfunc(m_func_mdid);

	m_efs = pmdfunc->EfsStability();
	m_efda = pmdfunc->EfdaDataAccess();
	m_fReturnsSet = pmdfunc->FReturnsSet();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::~CLogicalTVF
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLogicalTVF::~CLogicalTVF()
{
	CRefCount::SafeRelease(m_func_mdid);
	CRefCount::SafeRelease(m_return_type_mdid);
	CRefCount::SafeRelease(m_pdrgpcoldesc);
	CRefCount::SafeRelease(m_pdrgpcrOutput);
	GPOS_DELETE(m_pstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalTVF::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(
								COperator::HashValue(),
								gpos::CombineHashes(
										m_func_mdid->HashValue(),
										gpos::CombineHashes(
												m_return_type_mdid->HashValue(),
												gpos::HashPtr<DrgPcoldesc>(m_pdrgpcoldesc))));
	ulHash = gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));
	return ulHash;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalTVF::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalTVF *popTVF = CLogicalTVF::PopConvert(pop);
		
	return m_func_mdid->Equals(popTVF->FuncMdId()) &&
			m_return_type_mdid->Equals(popTVF->ReturnTypeMdId()) &&
			m_pdrgpcoldesc->Equals(popTVF->Pdrgpcoldesc()) &&
			m_pdrgpcrOutput->Equals(popTVF->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CLogicalTVF::PopCopyWithRemappedColumns
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

	CWStringConst *str = GPOS_NEW(memory_pool) CWStringConst(m_pstr->GetBuffer());
	m_func_mdid->AddRef();
	m_return_type_mdid->AddRef();
	m_pdrgpcoldesc->AddRef();

	return GPOS_NEW(memory_pool) CLogicalTVF(memory_pool, m_func_mdid, m_return_type_mdid, str, m_pdrgpcoldesc, pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalTVF::PcrsDeriveOutput
	(
	IMemoryPool *memory_pool,
	CExpressionHandle & // exprhdl
	)
{
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);
	pcrs->Include(m_pdrgpcrOutput);

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::PfpDerive
//
//	@doc:
//		Derive function properties
//
//---------------------------------------------------------------------------
CFunctionProp *
CLogicalTVF::PfpDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	BOOL fVolatileScan = (IMDFunction::EfsVolatile == m_efs);
	return PfpDeriveFromChildren(memory_pool, exprhdl, m_efs, m_efda, fVolatileScan, true /*fScan*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::FInputOrderSensitive
//
//	@doc:
//		Sensitivity to input order
//
//---------------------------------------------------------------------------
BOOL
CLogicalTVF::FInputOrderSensitive() const
{
	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalTVF::PxfsCandidates
	(
	IMemoryPool *memory_pool
	) 
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);

	(void) pxfs->ExchangeSet(CXform::ExfUnnestTVF);
	(void) pxfs->ExchangeSet(CXform::ExfImplementTVF);
	(void) pxfs->ExchangeSet(CXform::ExfImplementTVFNoArgs);
	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalTVF::Maxcard
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle & // exprhdl
	)
	const
{
	if (m_fReturnsSet)
	{
		// unbounded by default
		return CMaxCard();
	}

	return CMaxCard(1);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------

IStatistics *
CLogicalTVF::PstatsDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl,
	DrgPstat * // pdrgpstatCtxt
	)
	const
{
	CDouble dRows(1.0);
	if (m_fReturnsSet)
	{
		dRows = CStatistics::DDefaultRelationRows;
	}

	return PstatsDeriveDummy(memory_pool, exprhdl, dRows);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalTVF::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalTVF::OsPrint
	(
	IOstream &os
	)
	const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}
	os << SzId() << " (" << Pstr()->GetBuffer() << ") ";
	os << "Columns: [";
	CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
	os << "] ";
		
	return os;
}

// EOF

