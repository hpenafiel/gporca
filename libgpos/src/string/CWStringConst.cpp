//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CWStringConst.cpp
//
//	@doc:
//		Implementation of the wide character constant string class
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/common/clibwrapper.h"
#include "gpos/string/CWStringConst.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CWStringConst::CWStringConst
//
//	@doc:
//		Initializes a constant string with a given character buffer. The string
//		does not own the memory
//
//---------------------------------------------------------------------------
CWStringConst::CWStringConst
	(
	const WCHAR *wstrbuf
	)
	:
	CWStringBase
		(
		GPOS_WSZ_LENGTH(wstrbuf),
		false // owns_memory
		),
	m_wszBuf(wstrbuf)
{
	GPOS_ASSERT(NULL != wstrbuf);
	GPOS_ASSERT(IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CWStringConst::CWStringConst
//
//	@doc:
//		Initializes a constant string by making a copy of the given character buffer.
//		The string owns the memory.
//
//---------------------------------------------------------------------------
CWStringConst::CWStringConst
	(
	IMemoryPool *memory_pool,
	const WCHAR *wstrbuf
	)
	:
	CWStringBase
		(
		GPOS_WSZ_LENGTH(wstrbuf),
		true // owns_memory
		),
	m_wszBuf(NULL)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != wstrbuf);

	if (0 == m_length)
	{
		// string is empty
		m_wszBuf = &m_empty_wcstr;
	}
	else
	{
		// make a copy of the string
		WCHAR *wszTempBuf = GPOS_NEW_ARRAY(memory_pool, WCHAR, m_length + 1);
		clib::WcStrNCpy(wszTempBuf, wstrbuf, m_length + 1);
		m_wszBuf = wszTempBuf;
	}

	GPOS_ASSERT(IsValid());
}

//---------------------------------------------------------------------------
//	@function:
//		CWStringConst::CWStringConst
//
//	@doc:
//		Shallow copy constructor.
//
//---------------------------------------------------------------------------
CWStringConst::CWStringConst
	(
	const CWStringConst& str
	)
	:
	CWStringBase
		(
		str.Length(),
		false // owns_memory
		),
	m_wszBuf(str.GetBuffer())
{
	GPOS_ASSERT(NULL != m_wszBuf);
	GPOS_ASSERT(IsValid());
}
//---------------------------------------------------------------------------
//	@function:
//		CWStringConst::~CWStringConst
//
//	@doc:
//		Destroys a constant string. This involves releasing the character buffer
//		provided the string owns it.
//
//---------------------------------------------------------------------------
CWStringConst::~CWStringConst()
{
	if (m_owns_memory && m_wszBuf != &m_empty_wcstr)
	{
		GPOS_DELETE_ARRAY(m_wszBuf);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CWStringConst::GetBuffer
//
//	@doc:
//		Returns the wide character buffer
//
//---------------------------------------------------------------------------
const WCHAR*
CWStringConst::GetBuffer() const
{
	return m_wszBuf;
}

// EOF

