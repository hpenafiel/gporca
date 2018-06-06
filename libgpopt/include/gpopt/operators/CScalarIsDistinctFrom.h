//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CScalarIsDistinctFrom.h
//
//	@doc:
//		Is distinct from operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarIsDistinctFrom_H
#define GPOPT_CScalarIsDistinctFrom_H

#include "gpos/base.h"
#include "gpopt/operators/CScalarCmp.h"


namespace gpopt
{

	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CScalarIsDistinctFrom
	//
	//	@doc:
	//		Is distinct from operator
	//
	//---------------------------------------------------------------------------
	class CScalarIsDistinctFrom : public CScalarCmp
	{

		private:

			// private copy ctor
			CScalarIsDistinctFrom(const CScalarIsDistinctFrom &);

		public:

			// ctor
			CScalarIsDistinctFrom
				(
				IMemoryPool *memory_pool,
				IMDId *mdid_op,
				const CWStringConst *pstrOp
				)
				:
				CScalarCmp(memory_pool, mdid_op, pstrOp, IMDType::EcmptIDF)
			{
				GPOS_ASSERT(mdid_op->IsValid());
			}

			// dtor
			virtual
			~CScalarIsDistinctFrom()
			{}

			// ident accessors
			virtual
			EOperatorId Eopid() const
			{
				return EopScalarIsDistinctFrom;
			}

			// boolean expression evaluation
			virtual
			EBoolEvalResult Eber(ULongPtrArray *pdrgpulChildren) const;

			// return a string for operator name
			virtual
			const CHAR *SzId() const
			{
				return "CScalarIsDistinctFrom";
			}

			virtual
			BOOL FMatch(COperator *pop) const;

			// conversion function
			static
			CScalarIsDistinctFrom *PopConvert(COperator *pop);

			// get commuted scalar IDF operator
			virtual
			CScalarIsDistinctFrom *PopCommutedOp(IMemoryPool *memory_pool, COperator *pop);

	}; // class CScalarIsDistinctFrom

}

#endif // !GPOPT_CScalarIsDistinctFrom_H

// EOF
