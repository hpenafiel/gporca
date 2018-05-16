//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CHashMap.h
//
//	@doc:
//		Hash map
//		* stores deep objects, i.e., pointers
//		* equality == on key uses template function argument
//		* does not allow insertion of duplicates (no equality on value class req'd)
//		* destroys objects based on client-side provided destroy functions
//---------------------------------------------------------------------------
#ifndef GPOS_CHashMap_H
#define GPOS_CHashMap_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/CDynamicPtrArray.h"

namespace gpos
{	
	// fwd declaration
	template <class K, class T, 
		ULONG (*HashFn)(const K*), 
		BOOL (*EqFn)(const K*, const K*),
		void (*DestroyKFn)(K*),
		void (*DestroyTFn)(T*)>
	class CHashMapIter;
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CHashMap
	//
	//	@doc:
	//		Hash map
	//
	//---------------------------------------------------------------------------
	template <class K, class T, 
				ULONG (*HashFn)(const K*), 
				BOOL (*EqFn)(const K*, const K*),
				void (*DestroyKFn)(K*),
				void (*DestroyTFn)(T*)>
	class CHashMap : public CRefCount
	{
		// fwd declaration
		friend class CHashMapIter<K, T, HashFn, EqFn, DestroyKFn, DestroyTFn>;

		private:
		
			//---------------------------------------------------------------------------
			//	@class:
			//		CHashMapElem
			//
			//	@doc:
			//		Anchor for key/value pair
			//
			//---------------------------------------------------------------------------		
			class CHashMapElem
			{
				private:
				
					// key/value pair
					K *m_key;
					T *m_value;
					
					// own objects
					BOOL m_owns_objects;
					
					// private copy ctor
					CHashMapElem(const CHashMapElem &);
				
				public:
				
					// ctor
					CHashMapElem(K *key, T *value, BOOL fOwn)
                    :
                    m_key(key),
                    m_value(value),
                    m_owns_objects(fOwn)
                    {
                        GPOS_ASSERT(NULL != key);
                    }

					// dtor 
					~CHashMapElem()
                    {
                        // in case of a temporary hashmap element for lookup we do NOT own the
                        // objects, otherwise call destroy functions
                        if (m_owns_objects)
                        {
                            DestroyKFn(m_key);
                            DestroyTFn(m_value);
                        }
                    }

					// key accessor
					K *Pk() const
					{
						return m_key;
					}

					// value accessor
					T *Pt() const
					{
						return m_value;
					}
					
					// replace value
					void ReplaceValue(T *ptNew)
                    {
                        if (m_owns_objects)
                        {
                            DestroyTFn(m_value);
                        }
                        m_value = ptNew;
                    }

					// equality operator -- map elements are equal if their keys match
					BOOL operator == (const CHashMapElem &hme) const
					{
						return EqFn(m_key, hme.m_key);
					}
			};

			// memory pool
			IMemoryPool *const m_pmp;
			
			// size
			ULONG m_num_chains;
		
			// number of entries
			ULONG m_ulEntries;

			// each hash chain is an array of hashmap elements
			typedef CDynamicPtrArray<CHashMapElem, CleanupDelete> HashElemChain;
			HashElemChain **const m_chains;

			// array for keys
			// We use CleanupNULL because the keys are owned by the hash table
			typedef CDynamicPtrArray<K, CleanupNULL> Keys;
			Keys *const m_pdrgKeys;

			IntPtrArray *const m_filled_chains;

			// private copy ctor
			CHashMap(const CHashMap<K, T, HashFn, EqFn, DestroyKFn, DestroyTFn> &);
			
			// lookup appropriate hash chain in static table, may be NULL if
			// no elements have been inserted yet
			HashElemChain **GetChain(const K *key) const
			{
				GPOS_ASSERT(NULL != m_chains);
				return &m_chains[HashFn(key) % m_num_chains];
			}

			// clear elements
			void Clear()
            {
                for (ULONG i = 0; i < m_filled_chains->Size(); i++)
                {
                    // release each hash chain
                    m_chains[*(*m_filled_chains)[i]]->Release();
                }
                m_ulEntries = 0;
                m_filled_chains->Clear();
            }
	
			// lookup an element by its key
			void Lookup(const K *key, CHashMapElem **pphme) const
            {
                GPOS_ASSERT(NULL != pphme);

                CHashMapElem hme(const_cast<K*>(key), NULL /*T*/, false /*fOwn*/);
                CHashMapElem *phme = NULL;
                HashElemChain **ppdrgchain = GetChain(key);
                if (NULL != *ppdrgchain)
                {
                    phme = (*ppdrgchain)->Find(&hme);
                    GPOS_ASSERT_IMP(NULL != phme, *phme == hme);
                }

                *pphme = phme;
            }

		public:
		
			// ctor
			CHashMap<K, T, HashFn, EqFn, DestroyKFn, DestroyTFn> (IMemoryPool *pmp, ULONG ulSize = 128)
            :
            m_pmp(pmp),
            m_num_chains(ulSize),
            m_ulEntries(0),
            m_chains(GPOS_NEW_ARRAY(m_pmp, HashElemChain*, m_num_chains)),
            m_pdrgKeys(GPOS_NEW(m_pmp) Keys(m_pmp)),
            m_filled_chains(GPOS_NEW(pmp) IntPtrArray(pmp))
            {
                GPOS_ASSERT(ulSize > 0);
                (void) clib::PvMemSet(m_chains, 0, m_num_chains * sizeof(HashElemChain*));
            }

			// dtor
			~CHashMap<K, T, HashFn, EqFn, DestroyKFn, DestroyTFn> ()
            {
                // release all hash chains
                Clear();

                GPOS_DELETE_ARRAY(m_chains);
                m_pdrgKeys->Release();
                m_filled_chains->Release();
            }

			// insert an element if key is not yet present
			BOOL Insert(K *key, T *value)
            {
                if (NULL != Find(key))
                {
                    return false;
                }

                HashElemChain **ppdrgchain = GetChain(key);
                if (NULL == *ppdrgchain)
                {
                    *ppdrgchain = GPOS_NEW(m_pmp) HashElemChain(m_pmp);
                    INT iBucket = HashFn(key) % m_num_chains;
                    m_filled_chains->Append(GPOS_NEW(m_pmp) INT(iBucket));
                }

                CHashMapElem *phme = GPOS_NEW(m_pmp) CHashMapElem(key, value, true /*fOwn*/);
                (*ppdrgchain)->Append(phme);

                m_ulEntries++;

                m_pdrgKeys->Append(key);

                return true;
            }
			
			// lookup a value by its key
			T *Find(const K *key) const
            {
                CHashMapElem *phme = NULL;
                Lookup(key, &phme);
                if (NULL != phme)
                {
                    return phme->Pt();
                }

                return NULL;
            }

			// replace the value in a map entry with a new given value
			BOOL Replace(const K *key, T *ptNew)
            {
                GPOS_ASSERT(NULL != key);

                BOOL fSuccess = false;
                CHashMapElem *phme = NULL;
                Lookup(key, &phme);
                if (NULL != phme)
                {
                    phme->ReplaceValue(ptNew);
                    fSuccess = true;
                }

                return fSuccess;
            }

			// return number of map entries
			ULONG Size() const
			{
				return m_ulEntries;
			}		

	}; // class CHashMap

}

#endif // !GPOS_CHashMap_H

// EOF

