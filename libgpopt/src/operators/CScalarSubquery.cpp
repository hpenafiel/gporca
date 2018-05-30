//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarSubquery.cpp
//
//	@doc:
//		Implementation of scalar subqueries
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CScalarSubquery.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::CScalarSubquery
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CScalarSubquery::CScalarSubquery
	(
	IMemoryPool *memory_pool,
	const CColRef *pcr,
	BOOL fGeneratedByExist,
	BOOL fGeneratedByQuantified
	)
	: 
	CScalar(memory_pool),
	m_pcr(pcr),
	m_fGeneratedByExist(fGeneratedByExist),
	m_fGeneratedByQuantified(fGeneratedByQuantified)
{
	GPOS_ASSERT(NULL != pcr);
	GPOS_ASSERT(!(fGeneratedByExist && fGeneratedByQuantified));
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::~CScalarSubquery
//
//	@doc:
//		Destructor
//
//---------------------------------------------------------------------------
CScalarSubquery::~CScalarSubquery()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::MDIdType
//
//	@doc:
//		Type of scalar's value
//
//---------------------------------------------------------------------------
IMDId *
CScalarSubquery::MDIdType() const
{
	return m_pcr->Pmdtype()->MDId();
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CScalarSubquery::HashValue() const
{
	return gpos::CombineHashes(COperator::HashValue(), 
								gpos::HashPtr<CColRef>(m_pcr));
}

	
//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarSubquery::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarSubquery *popScalarSubquery = CScalarSubquery::PopConvert(pop);
		
		// match if computed columns are identical
		return popScalarSubquery->Pcr() == m_pcr &&
				popScalarSubquery->FGeneratedByQuantified() == m_fGeneratedByQuantified &&
				popScalarSubquery->FGeneratedByExist() == m_fGeneratedByExist;
	}
	
	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CScalarSubquery::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	CColRef *pcr = CUtils::PcrRemap(m_pcr, phmulcr, fMustExist);

	return GPOS_NEW(memory_pool) CScalarSubquery(memory_pool, pcr, m_fGeneratedByExist, m_fGeneratedByQuantified);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::PcrsUsed
//
//	@doc:
//		Locally used columns
//
//---------------------------------------------------------------------------
CColRefSet *
CScalarSubquery::PcrsUsed
	(
	IMemoryPool *memory_pool,
	CExpressionHandle &exprhdl
	)
{
	GPOS_ASSERT(1 == exprhdl.Arity());

	// used columns is an empty set unless subquery column is an outer reference
	CColRefSet *pcrs = GPOS_NEW(memory_pool) CColRefSet(memory_pool);

	CColRefSet *pcrsChildOutput = exprhdl.Pdprel(0 /* ulChildIndex */)->PcrsOutput();
	if (!pcrsChildOutput->FMember(m_pcr))
	{
		// subquery column is not produced by relational child, add it to used columns
		 pcrs->Include(m_pcr);
	}

	return pcrs;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::PpartinfoDerive
//
//	@doc:
//		Derive partition consumers
//
//---------------------------------------------------------------------------
CPartInfo *
CScalarSubquery::PpartinfoDerive
	(
	IMemoryPool *, // memory_pool, 
	CExpressionHandle &exprhdl
	)
	const
{
	CPartInfo *ppartinfoChild = exprhdl.Pdprel(0 /*ulChildIndex*/)->Ppartinfo();
	GPOS_ASSERT(NULL != ppartinfoChild);
	ppartinfoChild->AddRef();
	return ppartinfoChild;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubquery::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarSubquery::OsPrint
	(
	IOstream &os
	)
	const
{
	os	<< SzId() 
		<< "[";
	m_pcr->OsPrint(os);
	os	<< "]";

	if (m_fGeneratedByExist)
	{
		os << " generated by Exist SQ";
	}
	if (m_fGeneratedByQuantified)
	{
		os << " generated by Quantified SQ";
	}
	return os;
}


// EOF

