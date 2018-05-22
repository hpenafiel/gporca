//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009-2010 Greenplum Inc.
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMemoryPoolStatistics.h
//
//	@doc:
//		Statistics for memory pool.
//
//	@owner:
//
//	@test:
//
//---------------------------------------------------------------------------
#ifndef GPOS_CMemoryPoolStatistics_H
#define GPOS_CMemoryPoolStatistics_H

#include "gpos/types.h"


namespace gpos
{
	// Statistics for a memory pool
	class CMemoryPoolStatistics
	{
		private:

			ULLONG m_successful_allocations;

			ULLONG m_failed_allocations;

			ULLONG m_free;

			ULLONG m_live_obj;

			ULLONG m_live_obj_user_size;

			ULLONG m_live_obj_total_size;

			// private copy ctor
			CMemoryPoolStatistics(CMemoryPoolStatistics &);

		public:

			// ctor
			CMemoryPoolStatistics()
				:
				m_successful_allocations(0),
				m_failed_allocations(0),
				m_free(0),
				m_live_obj(0),
				m_live_obj_user_size(0),
				m_live_obj_total_size(0)
			 {}

			// dtor
			virtual ~CMemoryPoolStatistics()
			{}

			// get the total number of successful allocation calls
			ULLONG SuccessfulAllocations() const
			{
				return m_successful_allocations;
			}

			// get the total number of failed allocation calls
			ULLONG FailedAllocations() const
			{
				return m_failed_allocations;
			}

			// get the total number of free calls
			ULLONG Free() const
			{
				return m_free;
			}

			// get the number of live objects
			ULLONG LiveObj() const
			{
				return m_live_obj;
			}

			// get the user data size of live objects
			ULLONG LiveObjUserSize() const
			{
				return m_live_obj_user_size;
			}

			// get the total data size (user + header padding) of live objects;
			// not accounting for memory used by the underlying allocator for its header;
			ULLONG LiveObjTotalSize() const
			{
				return m_live_obj_total_size;
			}

			// record a successful allocation
			void RecordAllocation
				(
				ULONG user_data_size,
				ULONG total_data_size
				)
			{
				++m_successful_allocations;
				++m_live_obj;
				m_live_obj_user_size += user_data_size;
				m_live_obj_total_size += total_data_size;
			}

			// record a successful free call (of a valid, non-NULL pointer)
			void RecordFree
				(
				ULONG user_data_size,
				ULONG total_data_size
				)
			{
				++m_free;
				--m_live_obj;
				m_live_obj_user_size -= user_data_size;
				m_live_obj_total_size -= total_data_size;
			}

			// record a failed allocation attempt
			void RecordFailedAllocation()
			{
				++m_failed_allocations;
			}

			// return total allocated size
			virtual
			ULLONG TotalAllocatedSize() const
			{
				return m_live_obj_total_size;
			}

	}; // class CMemoryPoolStatistics
}

#endif // ! CMemoryPoolStatistics

// EOF

