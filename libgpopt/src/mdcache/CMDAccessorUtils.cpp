//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDAccessorUtils.cpp
//
//	@doc:
//		Implementation of the utility function associated with the metadata
//		accessor
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/error/CException.h"
#include "gpos/error/CErrorHandlerStandard.h"
#include "gpos/task/CAutoTraceFlag.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

#include "naucrates/exception.h"
#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/md/IMDAggregate.h"
#include "naucrates/md/IMDFunction.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpmd;
using namespace gpos;
using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessorUtils::PstrWindowFuncName
//
//	@doc:
//		Return the name of the window operation
//
//---------------------------------------------------------------------------
const CWStringConst *
CMDAccessorUtils::PstrWindowFuncName
	(
	CMDAccessor *md_accessor,
	IMDId *mdid_func
	)
{
	if (md_accessor->FAggWindowFunc(mdid_func))
	{
		const IMDAggregate *pmdagg = md_accessor->Pmdagg(mdid_func);
		
		return pmdagg->Mdname().GetMDName();
	}

	const IMDFunction *pmdfunc = md_accessor->Pmdfunc(mdid_func);

	return pmdfunc->Mdname().GetMDName();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessorUtils::PmdidWindowReturnType
//
//	@doc:
//		Return the return type of the window function
//
//---------------------------------------------------------------------------
IMDId *
CMDAccessorUtils::PmdidWindowReturnType
	(
	CMDAccessor *md_accessor,
	IMDId *mdid_func
	)
{

	if (md_accessor->FAggWindowFunc(mdid_func))
	{
		const IMDAggregate *pmdagg = md_accessor->Pmdagg(mdid_func);
		return pmdagg->PmdidTypeResult();
	}

	const IMDFunction *pmdfunc = md_accessor->Pmdfunc(mdid_func);
	return pmdfunc->PmdidTypeResult();
}


//---------------------------------------------------------------------------
//	@function:
//		CMDAccessorUtils::FCmpExists
//
//	@doc:
//		Does a scalar comparison object between given types exists
//
//---------------------------------------------------------------------------
BOOL
CMDAccessorUtils::FCmpExists
	(
	CMDAccessor *md_accessor,
	IMDId *pmdidLeft,
	IMDId *pmdidRight,
	IMDType::ECmpType ecmpt
	)
{
	GPOS_ASSERT(NULL != md_accessor);
	GPOS_ASSERT(NULL != pmdidLeft);
	GPOS_ASSERT(NULL != pmdidLeft);
	GPOS_ASSERT(IMDType::EcmptOther > ecmpt);

	CAutoTraceFlag atf1(EtraceSimulateOOM, false);
	CAutoTraceFlag atf2(EtraceSimulateAbort, false);
	CAutoTraceFlag atf3(EtraceSimulateIOError, false);
	CAutoTraceFlag atf4(EtraceSimulateNetError, false);

	if (pmdidLeft->Equals(pmdidRight))
	{
		const IMDType *pmdtypeLeft = md_accessor->Pmdtype(pmdidLeft);
		return IMDId::IsValid(pmdtypeLeft->PmdidCmp(ecmpt));
	}

	GPOS_TRY
	{
		(void) md_accessor->Pmdsccmp(pmdidLeft, pmdidRight, ecmpt);

		return true;
	}
	GPOS_CATCH_EX(ex)
	{
		GPOS_ASSERT(GPOS_MATCH_EX(ex, gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound));
		GPOS_RESET_EX;

		return false;
	}
	GPOS_CATCH_END;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessorUtils::FCastExists
//
//	@doc:
//		Does if a cast object between given source and destination types exists
//
//---------------------------------------------------------------------------
BOOL
CMDAccessorUtils::FCastExists
	(
	CMDAccessor *md_accessor,
	IMDId *pmdidSrc,
	IMDId *pmdidDest
	)
{
	GPOS_ASSERT(NULL != md_accessor);
	GPOS_ASSERT(NULL != pmdidSrc);
	GPOS_ASSERT(NULL != pmdidDest);

	CAutoTraceFlag atf1(EtraceSimulateOOM, false);
	CAutoTraceFlag atf2(EtraceSimulateAbort, false);
	CAutoTraceFlag atf3(EtraceSimulateIOError, false);
	CAutoTraceFlag atf4(EtraceSimulateNetError, false);

	GPOS_TRY
	{
		(void) md_accessor->Pmdcast(pmdidSrc, pmdidDest);

		return true;
	}
	GPOS_CATCH_EX(ex)
	{
		GPOS_ASSERT(GPOS_MATCH_EX(ex, gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound));
		GPOS_RESET_EX;

		return false;
	}
	GPOS_CATCH_END;
}


//---------------------------------------------------------------------------
//	@function:
//		CUtils::FScalarOpReturnsNullOnNullInput
//
//	@doc:
//		Does scalar operator return NULL on NULL input?
//
//---------------------------------------------------------------------------
BOOL
CMDAccessorUtils::FScalarOpReturnsNullOnNullInput
	(
	CMDAccessor *md_accessor,
	IMDId *mdid_op
	)
{
	GPOS_ASSERT(NULL != md_accessor);

	if (NULL == mdid_op || !mdid_op->IsValid())
	{
		// invalid mdid
		return false;
	}

	CAutoTraceFlag atf1(EtraceSimulateOOM, false);
	CAutoTraceFlag atf2(EtraceSimulateAbort, false);
	CAutoTraceFlag atf3(EtraceSimulateIOError, false);
	CAutoTraceFlag atf4(EtraceSimulateNetError, false);

	GPOS_TRY
	{
		const IMDScalarOp *pmdscop = md_accessor->Pmdscop(mdid_op);

		return pmdscop->FReturnsNullOnNullInput();
	}
	GPOS_CATCH_EX(ex)
	{
		GPOS_ASSERT(GPOS_MATCH_EX(ex, gpdxl::ExmaMD, gpdxl::ExmiMDCacheEntryNotFound));
		GPOS_RESET_EX;
	}
	GPOS_CATCH_END;

	return false;
}


//---------------------------------------------------------------------------
//	@function:
//		CUtils::FBoolType
//
//	@doc:
//		Return True if passed mdid is for BOOL type
//
//---------------------------------------------------------------------------
BOOL
CMDAccessorUtils::FBoolType
	(
	CMDAccessor *md_accessor,
	IMDId *mdid_type
	)
{
	GPOS_ASSERT(NULL != md_accessor);

	if (NULL != mdid_type && mdid_type->IsValid())
	{
		return (IMDType::EtiBool == md_accessor->Pmdtype(mdid_type)->Eti());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDAccessorUtils::FCommutativeScalarOp
//
//	@doc:
//		Is scalar operator commutative? This can be used with ScalarOp and ScalarCmp
//
//---------------------------------------------------------------------------
BOOL
CMDAccessorUtils::FCommutativeScalarOp
	(
	CMDAccessor *md_accessor,
	IMDId *mdid_op
	)
{
	GPOS_ASSERT(NULL != md_accessor);
	GPOS_ASSERT(NULL != mdid_op);

	const IMDScalarOp *pmdscop = md_accessor->Pmdscop(mdid_op);

	return mdid_op->Equals(pmdscop->PmdidOpCommute());
}


// EOF
