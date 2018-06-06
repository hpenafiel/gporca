//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalCTEAnchor.cpp
//
//	@doc:
//		Implementation of CTE anchor operator
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::CLogicalCTEAnchor
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalCTEAnchor::CLogicalCTEAnchor
	(
	IMemoryPool *memory_pool
	)
	:
	CLogical(memory_pool),
	m_id(0)
{
	m_fPattern = true;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::CLogicalCTEAnchor
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalCTEAnchor::CLogicalCTEAnchor
	(
	IMemoryPool *memory_pool,
	ULONG id
	)
	:
	CLogical(memory_pool),
	m_id(id)
{}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::PcrsDeriveOutput
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
CColRefSet *
CLogicalCTEAnchor::PcrsDeriveOutput
	(
	IMemoryPool *, // memory_pool
	CExpressionHandle &exprhdl
	)
{
	return PcrsDeriveOutputPassThru(exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
CKeyCollection *
CLogicalCTEAnchor::PkcDeriveKeys
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
//		CLogicalCTEAnchor::PpartinfoDerive
//
//	@doc:
//		Derive part consumer
//
//---------------------------------------------------------------------------
CPartInfo *
CLogicalCTEAnchor::PpartinfoDerive
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
	const
{
	CPartInfo *ppartinfoChild = exprhdl.Pdprel(0 /*ulChildIndex*/)->Ppartinfo();
	GPOS_ASSERT(NULL != ppartinfoChild);

	CExpression *pexprProducer = COptCtxt::PoctxtFromTLS()->Pcteinfo()->PexprCTEProducer(m_id);
	GPOS_ASSERT(NULL != pexprProducer);
	CPartInfo *ppartinfoCTEProducer = CDrvdPropRelational::Pdprel(pexprProducer->PdpDerive())->Ppartinfo();

	return CPartInfo::PpartinfoCombine(memory_pool, ppartinfoChild, ppartinfoCTEProducer);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::Maxcard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalCTEAnchor::Maxcard
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
//		CLogicalCTEAnchor::FMatch
//
//	@doc:
//		Match function
//
//---------------------------------------------------------------------------
BOOL
CLogicalCTEAnchor::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}

	CLogicalCTEAnchor *popCTEAnchor = CLogicalCTEAnchor::PopConvert(pop);

	return m_id == popCTEAnchor->Id();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::HashValue
//
//	@doc:
//		Hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalCTEAnchor::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), m_id);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
CXformSet *
CLogicalCTEAnchor::PxfsCandidates
	(
	IMemoryPool *memory_pool
	)
	const
{
	CXformSet *pxfs = GPOS_NEW(memory_pool) CXformSet(memory_pool);
	(void) pxfs->ExchangeSet(CXform::ExfCTEAnchor2Sequence);
	(void) pxfs->ExchangeSet(CXform::ExfCTEAnchor2TrivialSelect);
	return pxfs;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalCTEAnchor::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalCTEAnchor::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " (";
	os << m_id;
	os << ")";

	return os;
}

// EOF
