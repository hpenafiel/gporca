//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		ICostModelParams.h
//
//	@doc:
//		Interface for the parameters of the underlying cost model
//---------------------------------------------------------------------------



#ifndef GPOPT_ICostModelParams_H
#define GPOPT_ICostModelParams_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"


#include "naucrates/md/IMDRelation.h"
#include "CCost.h"

namespace gpopt
{
	using namespace gpos;
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		ICostModelParams
	//
	//	@doc:
	//		Interface for the parameters of the underlying cost model
	//
	//---------------------------------------------------------------------------
	class ICostModelParams : public CRefCount
	{
		public:

			//---------------------------------------------------------------------------
			//	@class:
			//		SCostParam
			//
			//	@doc:
			//		Internal structure to represent cost model parameter
			//
			//---------------------------------------------------------------------------
			struct SCostParam
			{

				private:

					// param identifier
					ULONG m_ulId;

					// param value
					CDouble m_value;

					// param lower bound
					CDouble m_lower_bound_val;

					// param upper bound
					CDouble m_upper_bound_val;

				public:

					// ctor
					SCostParam
						(
						ULONG ulId,
						CDouble dVal,
						CDouble dLowerBound,
						CDouble dUpperBound
						)
						:
						m_ulId(ulId),
						m_value(dVal),
						m_lower_bound_val(dLowerBound),
						m_upper_bound_val(dUpperBound)
					{
						GPOS_ASSERT(dVal >= dLowerBound);
						GPOS_ASSERT(dVal <= dUpperBound);
					}

					// dtor
					virtual
					~SCostParam()
					{};

					// return param identifier
					ULONG UlId() const
					{
						return m_ulId;
					}

					// return value
					CDouble Get() const
					{
						return m_value;
					}

					// return lower bound value
					CDouble GetLowerBoundVal() const
					{
						return m_lower_bound_val;
					}

					// return upper bound value
					CDouble GetUpperBoundVal() const
					{
						return m_upper_bound_val;
					}

					BOOL Equals(SCostParam *pcm) const
					{
						return UlId() == pcm->UlId() && Get() == pcm->Get() &&
							   GetLowerBoundVal() == pcm->GetLowerBoundVal() &&
							   GetUpperBoundVal() == pcm->GetUpperBoundVal();
					}

			}; // struct SCostParam

			// lookup param by id
			virtual
			SCostParam *PcpLookup(ULONG ulId) const = 0;

			// lookup param by name
			virtual
			SCostParam *PcpLookup(const CHAR *szName) const = 0;

			// set param by id
			virtual
			void SetParam(ULONG ulId, CDouble dVal, CDouble dLowerBound, CDouble dUpperBound) = 0;

			// set param by name
			virtual
			void SetParam(const CHAR *szName, CDouble dVal, CDouble dLowerBound, CDouble dUpperBound) = 0;

			// print function
			virtual
			IOstream &OsPrint(IOstream &os) const = 0;

			virtual BOOL
			Equals(ICostModelParams *pcm) const = 0;

			virtual const CHAR *
			SzNameLookup(ULONG ulId) const = 0;
	};
}

#endif // !GPOPT_ICostModelParams_H

// EOF
