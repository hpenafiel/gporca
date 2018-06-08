//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		IDatumBool.h
//
//	@doc:
//		Base abstract class for bool representation
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_IDatumBool_H
#define GPNAUCRATES_IDatumBool_H

#include "gpos/base.h"
#include "naucrates/base/IDatumStatisticsMappable.h"

namespace gpnaucrates
{
	//---------------------------------------------------------------------------
	//	@class:
	//		IDatumBool
	//
	//	@doc:
	//		Base abstract class for bool representation
	//
	//---------------------------------------------------------------------------
	class IDatumBool : public IDatumStatisticsMappable
	{

		private:

			// private copy ctor
			IDatumBool(const IDatumBool &);

		public:

			// ctor
			IDatumBool()
			{};

			// dtor
			virtual
			~IDatumBool()
			{};

			// accessor for datum type
			virtual IMDType::ETypeInfo Eti()
			{
				return IMDType::EtiBool;
			}

			// accessor of boolean value
			virtual
			BOOL FValue() const = 0;

			// can datum be mapped to a double
			BOOL IsDatumMappableToDouble() const
			{
				return true;
			}

			// map to double for stats computation
			CDouble GetDoubleMapping() const
			{
				if (FValue())
				{
					return CDouble(1.0);
				}
				
				return CDouble(0.0);
			}

			// can datum be mapped to LINT
			BOOL IsDatumMappableToLINT() const
			{
				return true;
			}

			// map to LINT for statistics computation
			LINT GetLINTMapping() const
			{
				if (FValue())
				{
					return LINT(1);
				}
				return LINT(0);
			}

			//  supports statistical comparisons based on the byte array representation of datum
			virtual
			BOOL FSupportsBinaryComp
				(
				const IDatum * //pdatum
				) 
				const
			{
				return false;
			}

			// byte array representation of datum
			virtual
			const BYTE *PbaVal() const
			{
				GPOS_ASSERT(!"Invalid invocation of PbaVal");
				return NULL;
			}

			// does the datum need to be padded before statistical derivation
			virtual
			BOOL FNeedsPadding() const
			{
				return false;
			}

			// return the padded datum
			virtual
			IDatum *PdatumPadded
				(
				IMemoryPool *, // memory_pool,
				ULONG    // col_len
				)
				const
			{
				GPOS_ASSERT(!"Invalid invocation of PdatumPadded");
				return NULL;
			}

			// statistics equality based on byte array representation of datums
			virtual
			BOOL FStatsEqualBinary
				(
				const IDatum * // pdatum
				)
				const
			{
				GPOS_ASSERT(!"Invalid invocation of FStatsEqualBinary");
				return false;
			}

			// statistics less than based on byte array representation of datums
			virtual
			BOOL FStatsLessThanBinary
				(
				const IDatum * //pdatum
				)
				const
			{
				GPOS_ASSERT(!"Invalid invocation of FStatsLessThanBinary");
				return false;
			}

			// does datum support like predicate
			virtual
			BOOL FSupportLikePredicate() const
			{
				return false;
			}

			// return the default scale factor of like predicate
			virtual
			CDouble DLikePredicateScaleFactor() const
			{
				GPOS_ASSERT(!"Invalid invocation of DLikeSelectivity");
				return false;
			}
	}; // class IDatumBool

}


#endif // !GPNAUCRATES_IDatumBool_H

// EOF
