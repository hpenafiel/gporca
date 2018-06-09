//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDefaultComparator.h
//
//	@doc:
//		Default comparator for IDatum instances
//
//	@owner:
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPOPT_CDefaultComparator_H
#define GPOPT_CDefaultComparator_H

#include "gpos/base.h"

#include "gpopt/base/IComparator.h"

#include "naucrates/md/IMDType.h"

#include "naucrates/traceflags/traceflags.h"

namespace gpmd
{
	// fwd declarations
	class IMDId;
}

namespace gpnaucrates
{
	// fwd declarations
	class IDatum;
}

namespace gpopt
{
	using namespace gpmd;
	using namespace gpnaucrates;
	using namespace gpos;

	// fwd declarations
	class IConstExprEvaluator;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDefaultComparator
	//
	//	@doc:
	//		Default comparator for IDatum instances. It is a singleton accessed
	//		via CompGetInstance.
	//
	//---------------------------------------------------------------------------
	class CDefaultComparator : public IComparator
	{
		private:
			// constant expression evaluator
			IConstExprEvaluator *m_pceeval;

			// disabled copy constructor
			CDefaultComparator(const CDefaultComparator &);

			// construct a comparison expression from the given components and evaluate it
			BOOL FEvalComparison
				(
				IMemoryPool *memory_pool,
				const IDatum *pdatum1,
				const IDatum *pdatum2,
				IMDType::ECmpType cmp_type
				)
				const;

			// return true iff we use built-in evaluation for integers
			static
			BOOL
			FUseBuiltinIntEvaluators()
			{
				return !GPOS_FTRACE(EopttraceEnableConstantExpressionEvaluation) ||
						!GPOS_FTRACE(EopttraceUseExternalConstantExpressionEvaluationForInts);
			}

		public:
			// ctor
			CDefaultComparator(IConstExprEvaluator *pceeval);

			// dtor
			virtual
			~CDefaultComparator()
			{}

			// tests if the two arguments are equal
			virtual
			BOOL Equals(const IDatum *pdatum1, const IDatum *pdatum2) const;

			// tests if the first argument is less than the second
			virtual
			BOOL FLessThan(const IDatum *pdatum1, const IDatum *pdatum2) const;

			// tests if the first argument is less or equal to the second
			virtual
			BOOL FLessThanOrEqual(const IDatum *pdatum1, const IDatum *pdatum2) const;

			// tests if the first argument is greater than the second
			virtual
			BOOL FGreaterThan(const IDatum *pdatum1, const IDatum *pdatum2) const;

			// tests if the first argument is greater or equal to the second
			virtual
			BOOL FGreaterThanOrEqual(const IDatum *pdatum1, const IDatum *pdatum2) const;

	};  // CDefaultComparator
}

#endif // !CDefaultComparator_H

// EOF
