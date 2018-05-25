//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		CWStringStatic.cpp
//
//	@doc:
//		Implementation of the wide character string class
//		with static buffer allocation
//---------------------------------------------------------------------------

#include "gpos/common/clibwrapper.h"
#include "gpos/string/CStringStatic.h"
#include "gpos/string/CWStringStatic.h"

using namespace gpos;

#define GPOS_STATIC_STR_BUFFER_LENGTH 5000


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::CWStringStatic
//
//	@doc:
//		Ctor - initializes with empty string
//
//---------------------------------------------------------------------------
CWStringStatic::CWStringStatic
	(
	WCHAR buffer[],
	ULONG capacity
	)
	:
	CWString
		(
		0 // length
		),
	capacity(capacity)
{
	GPOS_ASSERT(NULL != buffer);
	GPOS_ASSERT(0 < capacity);

	m_buffer = buffer;
	m_buffer[0] = WCHAR_EOS;
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::CWStringStatic
//
//	@doc:
//		Ctor - initializes with passed string
//
//---------------------------------------------------------------------------
CWStringStatic::CWStringStatic
	(
	WCHAR buffer[],
	ULONG capacity,
	const WCHAR init_str[]
	)
	:
	CWString
		(
		0 // length
		),
	capacity(capacity)
{
	GPOS_ASSERT(NULL != buffer);
	GPOS_ASSERT(0 < capacity);

	m_buffer = buffer;
	AppendBuffer(init_str);
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::AppendBuffer
//
//	@doc:
//		Appends the contents of a buffer to the current string
//
//---------------------------------------------------------------------------
void
CWStringStatic::AppendBuffer
	(
	const WCHAR *wstrbuf
	)
{
	GPOS_ASSERT(NULL != wstrbuf);
	ULONG length = GPOS_WSZ_LENGTH(wstrbuf);
	if (0 == length || capacity == m_length)
	{
		return;
	}
	
	// check if new length exceeds capacity
	if (capacity <= length + m_length)
	{
		// truncate string
		length = capacity - m_length - 1;
	}

	GPOS_ASSERT(capacity > length + m_length);

	clib::WcStrNCpy(m_buffer + m_length, wstrbuf, length + 1);
	m_length += length;
	
	// terminate string
	m_buffer[m_length] = WCHAR_EOS;

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::AppendWideCharArray
//
//	@doc:
//		Appends a null terminated wide character array
//
//---------------------------------------------------------------------------
void
CWStringStatic::AppendWideCharArray
	(
	const WCHAR *wchar_array
	)
{
	AppendBuffer(wchar_array);
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::AppendCharArray
//
//	@doc:
//		Appends a null terminated character array
//
//---------------------------------------------------------------------------
void
CWStringStatic::AppendCharArray
	(
	const CHAR *char_array
	)
{
	GPOS_ASSERT(NULL != char_array);
	if (0 ==  GPOS_SZ_LENGTH(char_array) || capacity == m_length)
	{
		return;
	}

	ULONG length = GPOS_SZ_LENGTH(char_array);
	if (length >= GPOS_STATIC_STR_BUFFER_LENGTH)
	{
		// if input string is larger than buffer length, use AppendFormat
		AppendFormat(GPOS_WSZ_LIT("%s"), char_array);
		return;
	}

	// otherwise, append to wide string character array directly
	WCHAR wstrbuf[GPOS_STATIC_STR_BUFFER_LENGTH];

	// convert input string to wide character buffer
	#ifdef GPOS_DEBUG
	ULONG ulLen =
	#endif // GPOS_DEBUG
		clib::MbToWcs(wstrbuf, char_array, length);
	GPOS_ASSERT(ulLen == length);

	// check if new length exceeds capacity
	if (capacity <= length + m_length)
	{
		// truncate string
		length = capacity - m_length - 1;
	}
	GPOS_ASSERT(capacity > length + m_length);

	// append input string to current end of buffer
	(void) clib::WcMemCpy(m_buffer + m_length, wstrbuf, length + 1);

	m_length += length;
	m_buffer[m_length] = WCHAR_EOS;

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::AppendFormat
//
//	@doc:
//		Appends a formatted string
//
//---------------------------------------------------------------------------
void
CWStringStatic::AppendFormat
	(
	const WCHAR *format,
	...
	)
{
	VA_LIST	va_args;

	// get arguments
	VA_START(va_args, format);

	AppendFormatVA(format, va_args);

	// reset arguments
	VA_END(va_args);
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::AppendFormatVA
//
//	@doc:
//		Appends a formatted string based on passed va list
//
//---------------------------------------------------------------------------
void
CWStringStatic::AppendFormatVA
	(
	const WCHAR *format,
	VA_LIST va_args
	)
{
	GPOS_ASSERT(NULL != format);

	// available space in buffer
	ULONG ulAvailable = capacity - m_length;

	// format string
	(void) clib::VswPrintf(m_buffer + m_length, ulAvailable, format, va_args);

	m_buffer[capacity - 1] = WCHAR_EOS;
	m_length = GPOS_WSZ_LENGTH(m_buffer);

	GPOS_ASSERT(capacity > m_length);

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::Reset
//
//	@doc:
//		Resets string
//
//---------------------------------------------------------------------------
void
CWStringStatic::Reset()
{
	m_buffer[0] = WCHAR_EOS;
	m_length = 0;
}


//---------------------------------------------------------------------------
//	@function:
//		CWStringStatic::AppendEscape
//
//	@doc:
//		Appends a string and replaces character with string
//
//---------------------------------------------------------------------------
void
CWStringStatic::AppendEscape
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

	ULONG length = str->Length();
	ULONG ulLengthReplace =  GPOS_WSZ_LENGTH(replace_str);
	ULONG ulLengthNew = m_length;
	const WCHAR *wsz = str->GetBuffer();

	for (ULONG i = 0; i < length && ulLengthNew < capacity - 1; i++)
	{
		if (wc == wsz[i] && !str->HasEscapedCharAt(i))
		{
			// check for overflow
			ULONG ulLengthCopy = std::min(ulLengthReplace, capacity - ulLengthNew - 1);

			clib::WcStrNCpy(m_buffer + ulLengthNew, replace_str, ulLengthCopy);
			ulLengthNew += ulLengthCopy;
		}
		else
		{
			m_buffer[ulLengthNew] = wsz[i];
			ulLengthNew++;
		}

		GPOS_ASSERT(ulLengthNew < capacity);
	}

	// terminate string
	m_buffer[ulLengthNew] = WCHAR_EOS;

	m_length = ulLengthNew;
	GPOS_ASSERT(IsValid());
}


// EOF

