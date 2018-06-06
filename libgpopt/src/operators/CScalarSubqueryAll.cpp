//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarSubqueryAll.cpp
//
//	@doc:
//		Implementation of scalar subquery ALL operator
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/operators/CScalarSubqueryAll.h"
#include "gpopt/base/CUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubqueryAll::CScalarSubqueryAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarSubqueryAll::CScalarSubqueryAll
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidScalarOp,
	const CWStringConst *pstrScalarOp,
	const CColRef *pcr
	)
	:
	CScalarSubqueryQuantified(memory_pool, pmdidScalarOp, pstrScalarOp, pcr)
{}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubqueryAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CScalarSubqueryAll::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	CColRef *pcr = CUtils::PcrRemap(Pcr(), phmulcr, fMustExist);

	IMDId *pmdidScalarOp = MdIdOp();
	pmdidScalarOp->AddRef();

	CWStringConst *pstrScalarOp = GPOS_NEW(memory_pool) CWStringConst(memory_pool, PstrOp()->GetBuffer());

	return GPOS_NEW(memory_pool) CScalarSubqueryAll(memory_pool, pmdidScalarOp, pstrScalarOp, pcr);
}

// EOF
