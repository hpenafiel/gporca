//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CEnumSet.h
//
//	@doc:
//		Implementation of set of enums as bitset
//---------------------------------------------------------------------------
#ifndef GPOS_CEnumSet_H
#define GPOS_CEnumSet_H

#include "gpos/base.h"
#include "gpos/common/CBitSet.h"


namespace gpos
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CEnumSet
	//
	//	@doc:
	//		Template derived from CBitSet
	//
	//---------------------------------------------------------------------------
	template<class T, ULONG eSentinel>
	class CEnumSet : public CBitSet
	{
	
		private:
	
			// hidden copy ctor
			CEnumSet<T, eSentinel>(const CEnumSet<T, eSentinel>&);
			
		public:
				
			// ctor
			explicit
			CEnumSet<T, eSentinel>(IMemoryPool *pmp)
				:
				CBitSet(pmp, eSentinel)
			{}
		
			explicit
			CEnumSet<T, eSentinel>(IMemoryPool *pmp, const CEnumSet<T, eSentinel> &pes)
				:
				CBitSet(pmp, pes)
			{}
			
			// dtor
			virtual ~CEnumSet<T, eSentinel>() {}
			
			// determine if bit is set
			BOOL Get(T t) const
			{
				GPOS_ASSERT(t >= 0);

				ULONG ulT = static_cast<ULONG>(t);
				GPOS_ASSERT(ulT < eSentinel && "Out of range of enum");
				
				return CBitSet::Get(ulT);
			}
			
			// set given bit; return previous value
			BOOL ExchangeSet(T t)
			{
				GPOS_ASSERT(t >= 0);

				ULONG ulT = static_cast<ULONG>(t);
				GPOS_ASSERT(ulT < eSentinel && "Out of range of enum");
				
				return CBitSet::ExchangeSet(ulT);
			}
		
			// clear given bit; return previous value
			BOOL ExchangeClear(T t)
			{
				GPOS_ASSERT(t >= 0);

				ULONG ulT = static_cast<ULONG>(t);
				GPOS_ASSERT(ulT < eSentinel && "Out of range of enum");
				
				return CBitSet::ExchangeClear(ulT);
			}

	}; // class CEnumSet
}

#endif // !GPOS_CEnumSet_H

// EOF

