//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CScalarIsDistinctFrom.cpp
//
//	@doc:
//		Implementation of scalar IDF comparison operator
//---------------------------------------------------------------------------

#include "gpopt/base/COptCtxt.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "gpopt/operators/CScalarIsDistinctFrom.h"

using namespace gpopt;
using namespace gpmd;


// conversion function
CScalarIsDistinctFrom *
CScalarIsDistinctFrom::PopConvert
(
 COperator *pop
 )
{
	GPOS_ASSERT(NULL != pop);
	GPOS_ASSERT(EopScalarIsDistinctFrom == pop->Eopid());
	
	return reinterpret_cast<CScalarIsDistinctFrom*>(pop);
}

// perform boolean expression evaluation
CScalar::EBoolEvalResult
CScalarIsDistinctFrom::Eber(ULongPtrArray *pdrgpulChildren) const
{
	GPOS_ASSERT(2 == pdrgpulChildren->Size());

	// Is Distinct From(IDF) expression will always evaluate
	// to a true/false/unknown but not a NULL
	EBoolEvalResult firstResult = (EBoolEvalResult) *(*pdrgpulChildren)[0];
	EBoolEvalResult secondResult = (EBoolEvalResult) *(*pdrgpulChildren)[1];

	if (firstResult == EberUnknown || secondResult == EberUnknown)
	{
		return CScalar::EberUnknown;

	}
	else if (firstResult != secondResult)
	{
		return CScalar::EberTrue;
	}
	return CScalar::EberFalse;
}

BOOL
CScalarIsDistinctFrom::FMatch
(
 COperator *pop
 )
const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarIsDistinctFrom *popIDF = CScalarIsDistinctFrom::PopConvert(pop);
		
		// match if operator mdids are identical
		return PmdidOp()->Equals(popIDF->PmdidOp());
	}
	
	return false;
}

// get commuted scalar IDF operator
CScalarIsDistinctFrom *
CScalarIsDistinctFrom::PopCommutedOp
	(
	IMemoryPool *memory_pool,
	COperator *pop
	)
{
	
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	IMDId *pmdid = PmdidCommuteOp(md_accessor, pop);
	if (NULL != pmdid && pmdid->IsValid())
	{
		return GPOS_NEW(memory_pool) CScalarIsDistinctFrom(memory_pool, pmdid, Pstr(memory_pool, md_accessor, pmdid));
	}
	return NULL;
}

// EOF

