//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CMDName.cpp
//
//	@doc:
//		Metadata name of objects
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"
#include "naucrates/md/CMDName.h"

using namespace gpmd;


//---------------------------------------------------------------------------
//	@function:
//		CMDName::CMDName
//
//	@doc:
//		Constructor
//		Creates a deep copy of the provided string
//
//---------------------------------------------------------------------------
CMDName::CMDName
	(
	IMemoryPool *memory_pool,
	const CWStringBase *str
	)
	:
	m_psc(NULL),
	m_fDeepCopy(true)
{
	m_psc = GPOS_NEW(memory_pool) CWStringConst(memory_pool, str->GetBuffer());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDName::CMDName
//
//	@doc:
//		ctor
//		Depending on the value of the the fOwnsMemory argument, the string object
//		can become property of the CMDName object
//
//---------------------------------------------------------------------------
CMDName::CMDName
	(
	const CWStringConst *str,
	BOOL fOwnsMemory
	)
	:
	m_psc(str),
	m_fDeepCopy(fOwnsMemory)
{
	GPOS_ASSERT(NULL != m_psc);
	GPOS_ASSERT(m_psc->IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDName::CMDName
//
//	@doc:
//		Shallow copy constructor
//
//---------------------------------------------------------------------------
CMDName::CMDName
	(
	const CMDName &name
	)
	:
	m_psc(name.GetMDName()),
	m_fDeepCopy(false)
{
	GPOS_ASSERT(NULL != m_psc->GetBuffer());
	GPOS_ASSERT(m_psc->IsValid());	
}


//---------------------------------------------------------------------------
//	@function:
//		CMDName::~CMDName
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CMDName::~CMDName()
{
	GPOS_ASSERT(m_psc->IsValid());

	if (m_fDeepCopy)
	{
		GPOS_DELETE(m_psc);
	}
}

// EOF

