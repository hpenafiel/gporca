//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CFSimulator.h
//
//	@doc:
//		Failpoint simulator framework; computes a hash value for current
//		call stack; if stack has not been seen before, stack repository 
//		returns true which makes the call macro simulate a failure, i.e.
//		throw an exception.
//---------------------------------------------------------------------------
#ifndef GPOS_CFSimulator_H
#define GPOS_CFSimulator_H

#include "gpos/types.h"

#if GPOS_FPSIMULATOR

#include "gpos/common/CBitVector.h"
#include "gpos/common/CList.h"
#include "gpos/common/CStackDescriptor.h"
#include "gpos/common/CSyncHashtable.h"
#include "gpos/common/CSyncHashtableAccessByKey.h"

// macro to trigger failure simulation; must be macro to get accurate 
// file/line information
#define GPOS_SIMULATE_FAILURE(trace, major, minor)	\
		do { \
			if (ITask::Self()->IsTraceSet(trace) && \
				CFSimulator::FSim()->NewStack(major, minor)) \
			{ \
				GPOS_RAISE(major, minor); \
			} \
		} while(0)

// resolution of hash vector
#define GPOS_FSIM_RESOLUTION 10000

	
namespace gpos
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CFSimulator
	//
	//	@doc:
	//		Failpoint simulator; maintains hashtable of stack hashes
	//
	//---------------------------------------------------------------------------
	class CFSimulator
	{	
	
		private:
		
			//---------------------------------------------------------------------------
			//	@class:
			//		CStackTracker
			//
			//	@doc:
			//		Tracks all stacks for a given exception, i.e. contains one single 
			//		bitvector; access to bitvector is protected by spinlock of hashtable
			//		in CFSimulator
			//
			//---------------------------------------------------------------------------
			class CStackTracker
			{
				public:
				
					//---------------------------------------------------------------------------
					//	@struct:
					//		StackKey
					//
					//	@doc:
					//		Wrapper around the two parts of an exception identification; provides
					//		equality operator for hashtable
					//
					//---------------------------------------------------------------------------
					struct StackKey
					{
						// stack trackers are identified by the exceptions they manage
						ULONG m_major;
						ULONG m_minor;
						
						// ctor
						StackKey
							(
							ULONG major,
							ULONG minor
							)
							:
							m_major(major),
							m_minor(minor)
							{}
						
						// simple comparison
						BOOL operator ==
							(
							const StackKey &key
							)
							const
						{
							return m_major == key.m_major && m_minor == key.m_minor;
						}

						// equality function -- needed for hashtable
						static
						BOOL Equals
							(
							const StackKey &key,
							const StackKey &other_key
							)
						{
							return key == other_key;
						}
						
						// basic hash function
						static
						ULONG HashValue
							(
							const StackKey &key
							)
						{
							return key.m_major ^ key.m_minor;
						}

					}; // struct StackKey
		
								
					// ctor
        			explicit
					CStackTracker(IMemoryPool *pmp, ULONG resolution, StackKey key);
					
					// exchange/set function
					BOOL ExchangeSet(ULONG bit);
					
					// link element for hashtable
					SLink m_link;

					// identifier
					StackKey m_key;

					// invalid key
					static
					const StackKey m_invalid_key;

				private:
				
					// no copy ctor
					CStackTracker(const CStackTracker &);

					// bitvector to hold stack hashes
					CBitVector *m_bit_vector;
										
			}; // class CStackTracker
		

		
			// hidden copy ctor
			CFSimulator(const CFSimulator&);

			// memory pool
			IMemoryPool *m_pmp;
			
			// resolution
			ULONG m_resolution;
			
			// short hands for stack repository and accessor
			typedef CSyncHashtable<CStackTracker, CStackTracker::StackKey, 
				CSpinlockOS> CStackTable;

			typedef CSyncHashtableAccessByKey<CStackTracker, CStackTracker::StackKey,
				CSpinlockOS> CStackTableAccessor;
				
			// stack repository
			CStackTable m_stack;

			// insert new tracker 
			void AddTracker(CStackTracker::StackKey key);

		public:
		
			// ctor
			CFSimulator(IMemoryPool *pmp, ULONG resolution);

			// dtor
			~CFSimulator() {}

			// determine if stack is new
			BOOL NewStack(ULONG major, ULONG minor);

			// global instance
			static
			CFSimulator *m_fsim;
			
			// initializer for global f-simulator
			static
			GPOS_RESULT Init();
			
#ifdef GPOS_DEBUG
			// destroy simulator
			void Shutdown();
#endif // GPOS_DEBUG
			
			// accessor for global instance
			static
			CFSimulator *FSim()
			{
				return m_fsim;
			}

			// check if simulation is activated
			static
			BOOL FSimulation()
			{
				ITask *task = ITask::Self();
				return
					task->IsTraceSet(EtraceSimulateOOM) ||
					task->IsTraceSet(EtraceSimulateAbort) ||
					task->IsTraceSet(EtraceSimulateIOError) ||
					task->IsTraceSet(EtraceSimulateNetError);
			}

	}; // class CFSimulator
}

#else

#define GPOS_SIMULATE_FAILURE(x,y)	;

#endif // !GPOS_FPSIMULATOR

#endif // !GPOS_CFSimulator_H

// EOF

