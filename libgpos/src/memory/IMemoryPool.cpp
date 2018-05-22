//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008-2010 Greenplum Inc.
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		IMemoryPool.cpp
//
//	@doc:
//		Implements functions that are used by the operators "new" and "delete"
//---------------------------------------------------------------------------

#include "gpos/error/CAutoTrace.h"
#include "gpos/error/CException.h"
#include "gpos/memory/CMemoryPool.h"
#include "gpos/memory/CMemoryPoolManager.h"


namespace gpos
{

ULONG
IMemoryPool::SizeOfAlloc(const void *ptr) {
	return CMemoryPool::SizeOfAlloc(ptr);
}

//---------------------------------------------------------------------------
//	@function:
//		IMemoryPool::NewImpl
//
//	@doc:
//		Implementation of New that can be used by "operator new" functions
////
//---------------------------------------------------------------------------
void*
IMemoryPool::NewImpl
	(
	SIZE_T size,
	const CHAR *filename,
	ULONG line,
	IMemoryPool::AllocationType eat
	)
{
	GPOS_ASSERT(ULONG_MAX >= size);
	GPOS_ASSERT_IMP
		(
		(NULL != CMemoryPoolManager::Pmpm()) && (this == CMemoryPoolManager::Pmpm()->PmpGlobal()),
		CMemoryPoolManager::Pmpm()->FAllowGlobalNew() &&
		"Use of new operator without target memory pool is prohibited, use New(...) instead"
		);

	ULONG alloc_size = CMemoryPool::UlAllocSize((ULONG) size);
	void *ptr = Allocate(alloc_size, filename, line);

	GPOS_OOM_CHECK(ptr);

	return dynamic_cast<CMemoryPool*>(this)->PvFinalizeAlloc(ptr, (ULONG) size, eat);
}

//---------------------------------------------------------------------------
//	@function:
//		DeleteImpl
//
//	@doc:
//		implementation of Delete that can be used by operator new functions
//
//---------------------------------------------------------------------------
void
IMemoryPool::DeleteImpl
	(
	void *ptr,
	AllocationType eat
	)
{
	// deletion of NULL pointers is legal
	if (NULL == ptr)
	{
		return;
	}

	// release allocation
	CMemoryPool::FreeAlloc(ptr, eat);
}

}  // namespace gpos

// EOF

