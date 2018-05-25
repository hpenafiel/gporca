//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CWStringDynamic.cpp
//
//	@doc:
//		Implementation of the wide character string class
//		with dynamic buffer allocation.
//---------------------------------------------------------------------------

#include "gpos/common/clibwrapper.h"
#include "gpos/string/CStringStatic.h"
#include "gpos/string/CWStringDynamic.h"
#include "gpos/common/CAutoRg.h"
using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::CWStringDynamic
//
//	@doc:
//		Constructs an empty string
//
//---------------------------------------------------------------------------
CWStringDynamic::CWStringDynamic
	(
	IMemoryPool *pmp
	)
	:
	CWString
		(
		0 // length
		),
	m_pmp(pmp),
	capacity(0)
{
	Reset();
}

//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::CWStringDynamic
//
//	@doc:
//		Constructs a new string and initializes it with the given buffer
//
//---------------------------------------------------------------------------
CWStringDynamic::CWStringDynamic
	(
	IMemoryPool *pmp,
	const WCHAR *wstrbuf
	)
	:
	CWString
		(
		GPOS_WSZ_LENGTH(wstrbuf)
		),
	m_pmp(pmp),
	capacity(0)
{
	GPOS_ASSERT(NULL != wstrbuf);

	Reset();
	AppendBuffer(wstrbuf);
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::~CWStringDynamic
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CWStringDynamic::~CWStringDynamic()
{
	Reset();
}


//---------------------------------------------------------------------------
//	@function:
//		CWString::Reset
//
//	@doc:
//		Resets string
//
//---------------------------------------------------------------------------
void
CWStringDynamic::Reset()
{
	if (NULL != m_buffer && &m_empty_wcstr != m_buffer)
	{
		GPOS_DELETE_ARRAY(m_buffer);
	}

	m_buffer = const_cast<WCHAR *>(&m_empty_wcstr);
	m_length = 0;
	capacity = 0;
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::AppendBuffer
//
//	@doc:
//		Appends the contents of a buffer to the current string
//
//---------------------------------------------------------------------------
void
CWStringDynamic::AppendBuffer
	(
	const WCHAR *wsz
	)
{
	GPOS_ASSERT(NULL != wsz);
	ULONG length = GPOS_WSZ_LENGTH(wsz);
	if (0 == length)
	{
		return;
	}

	// expand buffer if needed
	ULONG new_length = m_length + length;
	if (new_length + 1 > capacity)
	{
		IncreaseCapacity(new_length);
	}

	clib::WcStrNCpy(m_buffer + m_length, wsz, length + 1);
	m_length = new_length;

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::AppendWideCharArray
//
//	@doc:
//		Appends a a null terminated wide character array
//
//---------------------------------------------------------------------------
void
CWStringDynamic::AppendWideCharArray
	(
	const WCHAR *wchar_array
	)
{
	AppendBuffer(wchar_array);
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::AppendCharArray
//
//	@doc:
//		Appends a a null terminated character array
//
//---------------------------------------------------------------------------
void
CWStringDynamic::AppendCharArray
	(
	const CHAR *char_array
	)
{
	GPOS_ASSERT(NULL != char_array);

	// expand buffer if needed
	const ULONG length = GPOS_SZ_LENGTH(char_array);
	ULONG new_length = m_length + length;
	if (new_length + 1 > capacity)
	{
		IncreaseCapacity(new_length);
	}
	WCHAR *wstrbuf = GPOS_NEW_ARRAY(m_pmp, WCHAR, length + 1);

	// convert input string to wide character buffer
#ifdef GPOS_DEBUG
	ULONG len =
#endif // GPOS_DEBUG
		clib::MbToWcs(wstrbuf, char_array, length);
	GPOS_ASSERT(len == length);

	// append input string to current end of buffer
	(void) clib::WcMemCpy(m_buffer + m_length, wstrbuf, length + 1);
	GPOS_DELETE_ARRAY(wstrbuf);

	m_buffer[new_length] = WCHAR_EOS;
	m_length = new_length;

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::AppendFormat
//
//	@doc:
//		Appends a formatted string
//
//---------------------------------------------------------------------------
void
CWStringDynamic::AppendFormat
	(
	const WCHAR *format,
	...
	)
{
	GPOS_ASSERT(NULL != format);
	using clib::VswPrintf;

	VA_LIST	va_args;

	// determine length of format string after expansion
	INT res = -1;

	// attempt to fit the formatted string in a static array
	WCHAR static_buf[GPOS_WSTR_DYNAMIC_STATIC_BUFFER];

	// get arguments
	VA_START(va_args, format);

	// try expanding the formatted string in the buffer
	res = VswPrintf(static_buf, GPOS_ARRAY_SIZE(static_buf), format, va_args);

	// reset arguments
	VA_END(va_args);
	GPOS_ASSERT(-1 <= res);

	// estimated number of characters in expanded format string
	ULONG size = std::max(GPOS_WSZ_LENGTH(format), GPOS_ARRAY_SIZE(static_buf));

	// if the static buffer is too small, find the formatted string
	// length by trying to store it in a buffer of increasing size
	while (-1 == res)
	{
		// try with a bigger buffer this time
		size *= 2;
		CAutoRg<WCHAR> buf;
		buf = GPOS_NEW_ARRAY(m_pmp, WCHAR, size + 1);

		// get arguments
		VA_START(va_args, format);

		// try expanding the formatted string in the buffer
		res = VswPrintf(buf.Rgt(), size, format, va_args);

		// reset arguments
		VA_END(va_args);

		GPOS_ASSERT(-1 <= res);
	}
	// verify required buffer was not bigger than allowed
	GPOS_ASSERT(res >= 0);

	// expand buffer if needed
	ULONG new_length = m_length + ULONG(res);
	if (new_length + 1 > capacity)
	{
		IncreaseCapacity(new_length);
	}

	// get arguments
	VA_START(va_args, format);

	// print va_args to string
	VswPrintf(m_buffer + m_length, res + 1, format, va_args);

	// reset arguments
	VA_END(va_args);

	m_length = new_length;
	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::AppendEscape
//
//	@doc:
//		Appends a string and replaces character with string
//
//---------------------------------------------------------------------------
void
CWStringDynamic::AppendEscape
	(
	const CWStringBase *str,
	WCHAR wc,
	const WCHAR *replace_str
	)
{
	GPOS_ASSERT(NULL != str);

	if (str->IsEmpty())
	{
		return;
	}

	// count how many times the character to be escaped appears in the string
	ULONG occurrences = str->CountOccurrencesOf(wc);
	if (0 == occurrences)
	{
		Append(str);
		return;
	}

	ULONG length = str->Length();
	const WCHAR *strbuf = str->GetBuffer();

	ULONG replace_str_length =  GPOS_WSZ_LENGTH(replace_str);
	ULONG new_length = m_length + length + (replace_str_length - 1) * occurrences;
	if (new_length + 1 > capacity)
	{
		IncreaseCapacity(new_length);
	}

	// append new contents while replacing character with escaping string
	for (ULONG i = 0, j = m_length; i < length; i++)
	{
		if (wc == strbuf[i] && !str->HasEscapedCharAt(i))
		{
			clib::WcStrNCpy(m_buffer + j, replace_str, replace_str_length);
			j += replace_str_length;
		}
		else
		{
			m_buffer[j++] = strbuf[i];
		}
	}

	// terminate string
	m_buffer[new_length] = WCHAR_EOS;
	m_length = new_length;

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::IncreaseCapacity
//
//	@doc:
//		Increase string capacity
//
//---------------------------------------------------------------------------
void
CWStringDynamic::IncreaseCapacity
	(
	ULONG capacity
	)
{
	GPOS_ASSERT(capacity + 1 > capacity);

	ULONG new_capacity = UlCapacity(capacity + 1);
	GPOS_ASSERT(new_capacity > capacity + 1);
	GPOS_ASSERT(new_capacity >= (capacity << 1));

	CAutoRg<WCHAR> new_buffer;
	new_buffer = GPOS_NEW_ARRAY(m_pmp, WCHAR, new_capacity);
	if (0 < m_length)
	{
		// current string is not empty: copy it to the resulting string
		new_buffer = clib::WcStrNCpy(new_buffer.Rgt(), m_buffer, m_length);
	}

	// release old buffer
	if (m_buffer != &m_empty_wcstr)
	{
		GPOS_DELETE_ARRAY(m_buffer);
	}
	m_buffer = new_buffer.RgtReset();
	capacity = new_capacity;
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringDynamic::UlCapacity
//
//	@doc:
//		Find capacity that fits requested string size
//
//---------------------------------------------------------------------------
ULONG
CWStringDynamic::UlCapacity
	(
	ULONG size
	)
{
	ULONG capacity = GPOS_WSTR_DYNAMIC_CAPACITY_INIT;
	while (capacity <= size + 1)
	{
		capacity = capacity << 1;
	}

	return capacity;
}


// EOF

