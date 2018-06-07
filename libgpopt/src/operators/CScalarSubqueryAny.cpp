//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarSubqueryAny.cpp
//
//	@doc:
//		Implementation of scalar subquery ANY operator
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/operators/CScalarSubqueryAny.h"
#include "gpopt/base/CUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubqueryAny::CScalarSubqueryAny
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarSubqueryAny::CScalarSubqueryAny
	(
	IMemoryPool *memory_pool,
	IMDId *scalar_op_mdid,
	const CWStringConst *pstrScalarOp,
	const CColRef *pcr
	)
	:
	CScalarSubqueryQuantified(memory_pool, scalar_op_mdid, pstrScalarOp, pcr)
{}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubqueryAny::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
COperator *
CScalarSubqueryAny::PopCopyWithRemappedColumns
	(
	IMemoryPool *memory_pool,
	HMUlCr *phmulcr,
	BOOL fMustExist
	)
{
	CColRef *pcr = CUtils::PcrRemap(Pcr(), phmulcr, fMustExist);

	IMDId *scalar_op_mdid = MdIdOp();
	scalar_op_mdid->AddRef();

	CWStringConst *pstrScalarOp = GPOS_NEW(memory_pool) CWStringConst(memory_pool, PstrOp()->GetBuffer());

	return GPOS_NEW(memory_pool) CScalarSubqueryAny(memory_pool, scalar_op_mdid, pstrScalarOp, pcr);
}

// EOF
