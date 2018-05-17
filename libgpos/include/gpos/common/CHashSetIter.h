//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc
//
//	Hash set iterator

#ifndef GPOS_CHashSetIter_H
#define GPOS_CHashSetIter_H

#include "gpos/base.h"
#include "gpos/common/CStackObject.h"
#include "gpos/common/CHashSet.h"
#include "gpos/common/CDynamicPtrArray.h"

namespace gpos
{	

	// Hash set iterator
	template <class T,
				ULONG (*HashFn)(const T*), 
				BOOL (*EqFn)(const T*, const T*),
				void (*CleanupFn)(T*)>
	class CHashSetIter : public CStackObject
	{
	
		// short hand for hashset type
		typedef CHashSet<T, HashFn, EqFn, CleanupFn> TSet;
	
		private:

			// set to iterate
			const TSet *m_pts;

			// current hashchain
			ULONG m_ulChain;

			// current element
			ULONG m_ulElement;

			// is initialized?
			BOOL m_fInit;

			// private copy ctor
			CHashSetIter(const CHashSetIter<T, HashFn, EqFn, CleanupFn> &);

		public:
		
			// ctor
			CHashSetIter<T, HashFn, EqFn, CleanupFn> (TSet *pts)
            :
            m_pts(pts),
            m_ulChain(0),
            m_ulElement(0)
            {
                GPOS_ASSERT(NULL != pts);
            }

			// dtor
			virtual
			~CHashSetIter<T, HashFn, EqFn, CleanupFn> ()
			{}

			// advance iterator to next element
			BOOL Advance()
            {
                if (m_ulElement < m_pts->m_elements->Size())
                {
                    m_ulElement++;
                    return true;
                }

                return false;
            }

			// current element
			const T *Value() const
            {
				const typename TSet::CHashSetElem *phse = NULL;
				T *t = (*(m_pts->m_elements))[m_ulElement-1];
				phse = m_pts->Lookup(t);
                if (NULL != phse)
                {
                    return phse->Value();
                }
                return NULL;
            }

	}; // class CHashSetIter

}

#endif // !GPOS_CHashSetIter_H

// EOF

