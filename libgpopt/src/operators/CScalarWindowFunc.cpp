//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarWindowFunc.cpp
//
//	@doc:
//		Implementation of scalar window function call operators
//---------------------------------------------------------------------------

#include "gpos/base.h"

#include "naucrates/md/IMDAggregate.h"
#include "naucrates/md/IMDFunction.h"

#include "gpopt/base/CDrvdPropScalar.h"
#include "gpopt/base/CColRefSet.h"

#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/operators/CScalarFunc.h"
#include "gpopt/operators/CExpressionHandle.h"

#include "gpopt/mdcache/CMDAccessorUtils.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CScalarWindowFunc::CScalarWindowFunc
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarWindowFunc::CScalarWindowFunc
	(
	IMemoryPool *memory_pool,
	IMDId *mdid_func,
	IMDId *mdid_return_type,
	const CWStringConst *pstrFunc,
	EWinStage ewinstage,
	BOOL fDistinct,
	BOOL fStarArg,
	BOOL fSimpleAgg
	)
	:
	CScalarFunc(memory_pool),
	m_ewinstage(ewinstage),
	m_fDistinct(fDistinct),
	m_fStarArg(fStarArg),
	m_fSimpleAgg(fSimpleAgg),
	m_fAgg(false)
{
	GPOS_ASSERT(mdid_func->IsValid());
	GPOS_ASSERT(mdid_return_type->IsValid());
	m_func_mdid = mdid_func;
	m_return_type_mdid = mdid_return_type;
	m_pstrFunc = pstrFunc;

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	m_fAgg = md_accessor->FAggWindowFunc(m_func_mdid);
	if (!m_fAgg)
	{
		const IMDFunction *pmdfunc = md_accessor->Pmdfunc(m_func_mdid);
		m_efs = pmdfunc->EfsStability();
		m_efda = pmdfunc->EfdaDataAccess();
	}
	else
	{
 	 	// TODO: , Aug 15, 2012; pull out properties of aggregate functions
		m_efs = IMDFunction::EfsImmutable;
		m_efda = IMDFunction::EfdaNoSQL;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarWindowFunc::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CScalarWindowFunc::HashValue() const
{
	return gpos::CombineHashes
					(
					CombineHashes
						(
						CombineHashes
							(
							CombineHashes
								(
								gpos::CombineHashes
									(
									COperator::HashValue(),
									gpos::CombineHashes
										(
											m_func_mdid->HashValue(),
											m_return_type_mdid->HashValue()
										)
									),
								m_ewinstage
								),
							gpos::HashValue<BOOL>(&m_fDistinct)
							),
						gpos::HashValue<BOOL>(&m_fStarArg)
						),
					gpos::HashValue<BOOL>(&m_fSimpleAgg)
					);
}


//---------------------------------------------------------------------------
//	@function:
//		CScalarWindowFunc::FMatch
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CScalarWindowFunc::FMatch
	(
	COperator *pop
	)
	const
{
	if (pop->Eopid() == Eopid())
	{
		CScalarWindowFunc *popFunc = CScalarWindowFunc::PopConvert(pop);

		// match if the func id, and properties are identical
		return ((popFunc->FDistinct() ==  m_fDistinct)
				&& (popFunc->FStarArg() ==  m_fStarArg)
				&& (popFunc->FSimpleAgg() ==  m_fSimpleAgg)
				&& (popFunc->FAgg() == m_fAgg)
				&& m_func_mdid->Equals(popFunc->FuncMdId())
				&& m_return_type_mdid->Equals(popFunc->MDIdType())
				&& (popFunc->Ews() == m_ewinstage));
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarWindowFunc::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CScalarWindowFunc::OsPrint
	(
	IOstream &os
	)
	const
{
	os << SzId() << " (";
	os << PstrFunc()->GetBuffer();
	os << " , Agg: " << (m_fAgg ? "true" : "false");
	os << " , Distinct: " << (m_fDistinct ? "true" : "false");
	os << " , StarArgument: " << (m_fStarArg ? "true" : "false");
	os << " , SimpleAgg: " << (m_fSimpleAgg ? "true" : "false");
	os << ")";

	return os;
}

// EOF
