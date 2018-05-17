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
				ULONG (*pfnHash)(const T*), 
				BOOL (*pfnEq)(const T*, const T*),
				void (*CleanupFn)(T*)>
	class CHashSetIter : public CStackObject
	{
	
		// short hand for hashset type
		typedef CHashSet<T, pfnHash, pfnEq, CleanupFn> TSet;
	
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
			CHashSetIter(const CHashSetIter<T, pfnHash, pfnEq, CleanupFn> &);
			
			// method to return the current element
			const typename TSet::CHashSetElem *Phse() const
            {
                typename TSet::CHashSetElem *phse = NULL;
                T *t = (*(m_pts->m_pdrgElements))[m_ulElement-1];
                m_pts->Lookup(t, &phse);

                return phse;
            }

		public:
		
			// ctor
			CHashSetIter<T, pfnHash, pfnEq, CleanupFn> (TSet *pts)
            :
            m_pts(pts),
            m_ulChain(0),
            m_ulElement(0)
            {
                GPOS_ASSERT(NULL != pts);
            }

			// dtor
			virtual
			~CHashSetIter<T, pfnHash, pfnEq, CleanupFn> ()
			{}

			// advance iterator to next element
			BOOL Advance()
            {
                if (m_ulElement < m_pts->m_pdrgElements->Size())
                {
                    m_ulElement++;
                    return true;
                }

                return false;
            }

			// current element
			const T *Value() const
            {
                const typename TSet::CHashSetElem *phse = Phse();
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

