//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CSyncHashtable.h
//
//	@doc:
//		Allocation-less static hashtable; 
//		Manages client objects without additional allocations; this is a
//		requirement for system programming tasks to ensure the hashtable
//		works in exception situations, e.g. OOM;
//
//		1)	Hashtable is static and cannot resize during operations;
//		2)	expects target type to have SLink (see CList.h) and Key
//			members with appopriate accessors;
//		3)	clients must provide their own hash function;
//		4)	hashtable synchronizes through spinlocks on each bucket;
//---------------------------------------------------------------------------
#ifndef GPOS_CSyncHashtable_H
#define GPOS_CSyncHashtable_H

#include "gpos/base.h"

#include "gpos/common/CAutoRg.h"
#include "gpos/common/CList.h"
#include "gpos/sync/CAutoSpinlock.h"
#include "gpos/task/CAutoSuspendAbort.h"

namespace gpos
{

	// prototypes
	template <class T, class K, class S>
	class CSyncHashtableAccessorBase;

	template <class T, class K, class S>
	class CSyncHashtableAccessByKey;

	template <class T, class K, class S>
	class CSyncHashtableIter;

	template <class T, class K, class S>
	class CSyncHashtableAccessByIter;

	//---------------------------------------------------------------------------
	//	@class:
	//		CSyncHashtable<T, K, S>
	//
	//	@doc:
	//		Allocation-less static hash table;
	//
	//		Ideally the offset of the key would be a template parameter too in order
	//		to avoid accidental tampering with this value -- not all compiler allow 
	//		the use of the offset macro in the template definition, however.
	//
	//---------------------------------------------------------------------------
	template <class T, class K, class S>
	class CSyncHashtable
	{
		// accessor and iterator classes are friends
		friend class CSyncHashtableAccessorBase<T, K, S>;
		friend class CSyncHashtableAccessByKey<T, K, S>;
		friend class CSyncHashtableAccessByIter<T, K, S>;
		friend class CSyncHashtableIter<T, K, S>;

		private:
	
			// hash bucket is a pair of list, spinlock
			struct SBucket
			{
				private:
			
					// no copy ctor
					SBucket(const SBucket &);

				public:
			
					// ctor
					SBucket() {};
				
					// spinlock to protect bucket
					S m_lock;
				
					// hash chain
					CList<T> m_list;

#ifdef GPOS_DEBUG
					// bucket number
					ULONG m_bucket_idx;
#endif // GPOS_DEBUG

			};

			// range of buckets
			SBucket *m_buckets;
		
			// number of ht buckets
			ULONG m_nbuckets;

			// number of ht entries
			volatile ULONG_PTR m_size;
		
			// offset of key
			ULONG m_key_offset;

			// invalid key - needed for iteration
			const K *m_invalid_key;

			// pointer to hashing function
			ULONG (*m_hashfn)(const K&);

			// pointer to key equality function
			BOOL (*m_eqfn)(const K&, const K&);
				
			// function to compute bucket index for key
			ULONG GetBucketIndex
				(
				const K &key
				)
				const
			{
				GPOS_ASSERT(IsValid(key) && "Invalid key is inaccessible");

				return m_hashfn(key) % m_nbuckets;
			}

			// function to get bucket by index
			SBucket &GetBucket
				(
				const ULONG ulIndex
				)
				const
			{
				GPOS_ASSERT(ulIndex < m_nbuckets && "Invalid bucket index");

				return m_buckets[ulIndex];
			}

			// extract key out of type
			K &Key
				(
				T *pt
				)
				const
			{
				GPOS_ASSERT(ULONG_MAX != m_key_offset && "Key offset not initialized.");
			
				K &k = *(K*)((BYTE*)pt + m_key_offset);

				return k;
			}

			// key validity check
			BOOL IsValid
				(
				const K &key
				)
				const
			{
				return !m_eqfn(key, *m_invalid_key);
			}

		public:
	
			// type definition of function used to cleanup element
			typedef void (*DestroyEntryFuncPtr)(T *);

			// ctor
			CSyncHashtable<T, K, S>()
				:
				m_buckets(NULL),
				m_nbuckets(0),
				m_size(0),
				m_key_offset(ULONG_MAX),
				m_invalid_key(NULL)
			{}
		
			// dtor
			// deallocates hashtable internals, does not destroy
			// client objects
			~CSyncHashtable<T, K, S>()
            {
                Cleanup();
            }
		
			// Initialization of hashtable
			void Init
				(
				IMemoryPool *pmp,
				ULONG cSize,
				ULONG cLinkOffset,
				ULONG cKeyOffset,
				const K *pkeyInvalid,
				ULONG (*pfuncHash)(const K&),
				BOOL (*pfuncEqual)(const K&, const K&)
				)
            {
                GPOS_ASSERT(NULL == m_buckets);
                GPOS_ASSERT(0 == m_nbuckets);
                GPOS_ASSERT(NULL != pkeyInvalid);
                GPOS_ASSERT(NULL != pfuncHash);
                GPOS_ASSERT(NULL != pfuncEqual);

                m_nbuckets = cSize;
                m_key_offset = cKeyOffset;
                m_invalid_key = pkeyInvalid;
                m_hashfn = pfuncHash;
                m_eqfn = pfuncEqual;

                m_buckets = GPOS_NEW_ARRAY(pmp, SBucket, m_nbuckets);

                // NOTE: 03/25/2008; since it's the only allocation in the
                //		constructor the protection is not needed strictly speaking;
                //		Using auto range here just for cleanliness;
                CAutoRg<SBucket> argbucket;
                argbucket = m_buckets;

                for(ULONG i = 0; i < m_nbuckets; i ++)
                {
                    m_buckets[i].m_list.Init(cLinkOffset);
    #ifdef GPOS_DEBUG
                    // add serial number
                    m_buckets[i].m_bucket_idx = i;
    #endif // GPOS_DEBUG
                }

                // unhook from protector
                argbucket.RgtReset();
            }

			// dealloc bucket range and reset members
			void Cleanup()
            {
                GPOS_DELETE_ARRAY(m_buckets);
                m_buckets = NULL;

                m_nbuckets = 0;
            }

			// iterate over all entries and call destroy function on each entry
			void DestroyEntries(DestroyEntryFuncPtr pfuncDestroy)
            {
                // need to suspend cancellation while cleaning up
                CAutoSuspendAbort asa;

                T *pt = NULL;
                CSyncHashtableIter<T, K, S> shtit(*this);

                // since removing an entry will automatically advance iter's
                // position, we need to make sure that advance iter is called
                // only when we do not have an entry to delete
                while (NULL != pt || shtit.Advance())
                {
                    if (NULL != pt)
                    {
                        pfuncDestroy(pt);
                    }

                    {
                        CSyncHashtableAccessByIter<T, K, S> shtitacc(shtit);
                        if (NULL != (pt = shtitacc.Value()))
                        {
                            shtitacc.Remove(pt);
                        }
                    }
                }

    #ifdef GPOS_DEBUG
                CSyncHashtableIter<T, K, S> shtitSnd(*this);
                GPOS_ASSERT(!shtitSnd.Advance());
    #endif // GPOS_DEBUG
            }

			// insert function;
			void Insert(T *pt)
            {
                K &key = Key(pt);

                GPOS_ASSERT(IsValid(key));

                // determine target bucket
                SBucket &bucket = GetBucket(GetBucketIndex(key));

                // acquire auto spinlock
                CAutoSpinlock alock(bucket.m_lock);
                alock.Lock();

                // inserting at bucket's head is required by hashtable iteration
                bucket.m_list.Prepend(pt);

                // increase number of entries
                (void) UlpExchangeAdd(&m_size, 1);
            }

			// return number of entries
			ULONG_PTR UlpEntries() const
			{
				return m_size;
			}

	}; // class CSyncHashtable

}

#endif // !GPOS_CSyncHashtable_H

// EOF

