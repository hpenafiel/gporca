//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CStringStatic.cpp
//
//	@doc:
//		Implementation of the ASCII-character String class
//		with static buffer allocation
//---------------------------------------------------------------------------

#include "gpos/common/clibwrapper.h"
#include "gpos/string/CStringStatic.h"
#include "gpos/string/CWStringStatic.h"



using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::CStringStatic
//
//	@doc:
//		Ctor - initializes with empty string
//
//---------------------------------------------------------------------------
CStringStatic::CStringStatic
	(
	CHAR szBuffer[],
	ULONG capacity
	)
	:
	m_szBuf(szBuffer),
	m_length(0),
	capacity(capacity)
{
	GPOS_ASSERT(NULL != szBuffer);
	GPOS_ASSERT(0 < capacity);

	m_szBuf[0] = CHAR_EOS;
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::CStringStatic
//
//	@doc:
//		Ctor with string initialization
//
//---------------------------------------------------------------------------
CStringStatic::CStringStatic
	(
	CHAR buffer[],
	ULONG capacity,
	const CHAR init_str[]
	)
	:
	m_szBuf(buffer),
	m_length(0),
	capacity(capacity)
{
	GPOS_ASSERT(NULL != buffer);
	GPOS_ASSERT(0 < capacity);

	AppendBuffer(init_str);
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::Equals
//
//	@doc:
//		Checks whether the string is byte-wise equal to a given string literal
//
//---------------------------------------------------------------------------
BOOL
CStringStatic::Equals
	(
	const CHAR *buf
	)
	const
{
	GPOS_ASSERT(NULL != buf);

	ULONG length = clib::StrLen(buf);
	return (m_length == length && 0 == clib::StrNCmp(m_szBuf, buf, length));
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::Append
//
//	@doc:
//		Appends a string
//
//---------------------------------------------------------------------------
void
CStringStatic::Append
	(
	const CStringStatic *str
	)
{
	AppendBuffer(str->Sz());
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::AppendBuffer
//
//	@doc:
//		Appends the contents of a buffer to the current string
//
//---------------------------------------------------------------------------
void
CStringStatic::AppendBuffer
	(
	const CHAR *buf
	)
{
	GPOS_ASSERT(NULL != buf);
	ULONG length = clib::StrLen(buf);
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

	clib::StrNCpy(m_szBuf + m_length, buf, length + 1);
	m_length += length;

	// terminate string
	m_szBuf[m_length] = CHAR_EOS;

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::AppendFormat
//
//	@doc:
//		Appends a formatted string
//
//---------------------------------------------------------------------------
void
CStringStatic::AppendFormat
	(
	const CHAR *format,
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
//		CStringStatic::AppendFormatVA
//
//	@doc:
//		Appends a formatted string based on passed va list
//
//---------------------------------------------------------------------------
void
CStringStatic::AppendFormatVA
	(
	const CHAR *format,
	VA_LIST va_args
	)
{
	GPOS_ASSERT(NULL != format);

	// available space in buffer
	ULONG ulAvailable = capacity - m_length;

	// format string
	(void) clib::VsnPrintf(m_szBuf + m_length, ulAvailable, format, va_args);

	// terminate string
	m_szBuf[capacity - 1] = CHAR_EOS;
	m_length = clib::StrLen(m_szBuf);

	GPOS_ASSERT(capacity > m_length);

	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::AppendWsz
//
//	@doc:
//		Appends wide character string
//
//---------------------------------------------------------------------------
void
CStringStatic::AppendConvert
	(
	const WCHAR *wsz
	)
{
	ULONG ulLengthEntry = GPOS_WSZ_LENGTH(wsz);

	if (capacity - m_length < ulLengthEntry)
	{
		ulLengthEntry = capacity - m_length - 1;
	}

	for (ULONG i = 0; i < ulLengthEntry; i++)
	{
		CHAR szConvert[MB_LEN_MAX];

		/* convert wide character to multi-byte array */
		ULONG ulCharLen = clib::WcToMb(szConvert, wsz[i]);
		GPOS_ASSERT(0 < ulCharLen);

		// check if wide character is ASCII-compatible
		if (1 == ulCharLen)
		{
			// simple cast; works only for ASCII characters
			m_szBuf[m_length] = CHAR(wsz[i]);
		}
		else
		{
			// substitute wide character
			m_szBuf[m_length] = GPOS_WCHAR_UNPRINTABLE;
		}
		m_length++;
	}

	m_szBuf[m_length] = CHAR_EOS;
	GPOS_ASSERT(IsValid());
}


//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::Reset
//
//	@doc:
//		Resets string
//
//---------------------------------------------------------------------------
void
CStringStatic::Reset()
{
	m_szBuf[0] = CHAR_EOS;
	m_length = 0;
}


#ifdef GPOS_DEBUG

//---------------------------------------------------------------------------
//	@function:
//		CStringStatic::IsValid
//
//	@doc:
//		Checks whether a string is properly null-terminated
//
//---------------------------------------------------------------------------
bool
CStringStatic::IsValid() const
{
	return (m_length == clib::StrLen(m_szBuf));
}

#endif // GPOS_DEBUG


// EOF

