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
	m_buffer(wstrbuf)
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
	IMemoryPool *pmp,
	const WCHAR *wstrbuf
	)
	:
	CWStringBase
		(
		GPOS_WSZ_LENGTH(wstrbuf),
		true // owns_memory
		),
	m_buffer(NULL)
{
	GPOS_ASSERT(NULL != pmp);
	GPOS_ASSERT(NULL != wstrbuf);

	if (0 == m_length)
	{
		// string is empty
		m_buffer = &m_empty_wcstr;
	}
	else
	{
		// make a copy of the string
		WCHAR *temp_buf = GPOS_NEW_ARRAY(pmp, WCHAR, m_length + 1);
		clib::WcStrNCpy(temp_buf, wstrbuf, m_length + 1);
		m_buffer = temp_buf;
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
	m_buffer(str.GetBuffer())
{
	GPOS_ASSERT(NULL != m_buffer);
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
	if (m_owns_memory && m_buffer != &m_empty_wcstr)
	{
		GPOS_DELETE_ARRAY(m_buffer);
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
	return m_buffer;
}

// EOF

