//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CSyncList.h
//
//	@doc:
//		Template-based synchronized list class with minimum synchronization
//		overhead; it provides a minimal set of thread-safe operations through
//		atomic primitives (lock-free);
//
//		It requires that the elements are only inserted once, as their address
//		is used for identification; if elements are concurrently deleted,
//		iteration through the list is not thread safe;
//
//		In order to be useful for system programming the class must be
//		allocation-less, i.e. manage elements without additional allocation,
//		to work in exception or OOM situations;
//---------------------------------------------------------------------------
#ifndef GPOS_CSyncList_H
#define GPOS_CSyncList_H

#include "gpos/types.h"

#include "gpos/common/CList.h"
#include "gpos/sync/atomic.h"

namespace gpos
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CSyncList<class T>
	//
	//	@doc:
	//		Allocation-less list class; requires T to have a pointer to T embedded
	//		and N being the offset of this member;
	//
	//---------------------------------------------------------------------------
	template<class T>
	class CSyncList
	{
		private:

			// underlying list
			CList<T> m_list;

			// back-off after too many attempts
			void CheckBackOff(ULONG &ulAttempts) const
            {
                if (++ulAttempts == GPOS_SPIN_ATTEMPTS)
                {
                    // back-off
                    clib::USleep(GPOS_SPIN_BACKOFF);

                    ulAttempts = 0;

                    GPOS_CHECK_ABORT;
                }
            }

			// no copy ctor
			CSyncList(const CSyncList&);

		public:

			// ctor
			CSyncList()
			{}

			// dtor
			~CSyncList()
            {}

			// init function to facilitate arrays
			void Init
				(
				ULONG ulOffset
				)
			{
				m_list.Init(ulOffset);
			}

			// insert element at the head of the list;
			void Push(T *pt)
            {
                GPOS_ASSERT(NULL != pt);
                GPOS_ASSERT(m_list.First() != pt);

                ULONG ulAttempts = 0;
                SLink &linkElem = m_list.Link(pt);

    #ifdef GPOS_DEBUG
                void *pvHeadNext = linkElem.m_next;
    #endif // GPOS_DEBUG

                GPOS_ASSERT(NULL == linkElem.m_next);

                // keep spinning until passed element becomes the head
                while (true)
                {
                    T *ptHead = m_list.First();

                    GPOS_ASSERT(pt != ptHead && "Element is already inserted");
                    GPOS_ASSERT(pvHeadNext == linkElem.m_next && "Element is concurrently accessed");

                    // set current head as next element
                    linkElem.m_next = ptHead;
    #ifdef GPOS_DEBUG
                    pvHeadNext = linkElem.m_next;
    #endif // GPOS_DEBUG

                    // attempt to set element as head
                    if (FCompareSwap<T>((volatile T**) &m_list.m_head, ptHead, pt))
                    {
                        break;
                    }

                    CheckBackOff(ulAttempts);
                }
            }

			// remove element from the head of the list;
			T *Pop()
            {
                ULONG ulAttempts = 0;
                T *ptHeadOld = NULL;

                // keep spinning until the head is removed
                while (true)
                {
                    // get current head
                    ptHeadOld = m_list.First();
                    if (NULL == ptHeadOld)
                    {
                        break;
                    }

                    // second element becomes the new head
                    SLink &linkElem = m_list.Link(ptHeadOld);
                    T *ptHeadNew = static_cast<T*>(linkElem.m_next);

                    // attempt to set new head
                    if (FCompareSwap<T>((volatile T**) &m_list.m_head, ptHeadOld, ptHeadNew))
                    {
                        // reset link
                        linkElem.m_next = NULL;
                        break;
                    }

                    CheckBackOff(ulAttempts);
                }

                return ptHeadOld;
            }

			// get first element
			T *PtFirst()
			{
				m_list.m_tail = m_list.m_head;
				return m_list.First();
			}

			// get next element
			T *PtNext
				(
				T *pt
				)
			{
				m_list.m_tail = m_list.m_head;
				return m_list.Next(pt);
			}

			// check if list is empty
			BOOL IsEmpty() const
			{
				return NULL == m_list.First();
			}

#ifdef GPOS_DEBUG

			// lookup a given element in the stack
			// this works only when no elements are removed
			GPOS_RESULT
			EresFind
				(
				T *pt
				)
			{
				m_list.m_tail = m_list.m_head;
				return m_list.Find(pt);
			}

			// debug print of element addresses
			// this works only when no elements are removed
			IOstream &
			OsPrint
				(
				IOstream &os
				)
				const
			{
				return m_list.OsPrint(os);
			}

#endif // GPOS_DEBUG

	}; // class CSyncList
}

#endif // !GPOS_CSyncList_H


// EOF
