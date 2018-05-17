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
	ULONG ulCapacity
	)
	:
	m_szBuf(szBuffer),
	m_ulLength(0),
	m_ulCapacity(ulCapacity)
{
	GPOS_ASSERT(NULL != szBuffer);
	GPOS_ASSERT(0 < m_ulCapacity);

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
	CHAR szBuffer[],
	ULONG ulCapacity,
	const CHAR szInit[]
	)
	:
	m_szBuf(szBuffer),
	m_ulLength(0),
	m_ulCapacity(ulCapacity)
{
	GPOS_ASSERT(NULL != szBuffer);
	GPOS_ASSERT(0 < m_ulCapacity);

	AppendBuffer(szInit);
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

	ULONG ulLength = clib::StrLen(buf);
	return (m_ulLength == ulLength && 0 == clib::StrNCmp(m_szBuf, buf, ulLength));
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
	const CStringStatic *pstr
	)
{
	AppendBuffer(pstr->Sz());
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
	ULONG ulLength = clib::StrLen(buf);
	if (0 == ulLength || m_ulCapacity == m_ulLength)
	{
		return;
	}

	// check if new length exceeds capacity
	if (m_ulCapacity <= ulLength + m_ulLength)
	{
		// truncate string
		ulLength = m_ulCapacity - m_ulLength - 1;
	}

	GPOS_ASSERT(m_ulCapacity > ulLength + m_ulLength);

	clib::StrNCpy(m_szBuf + m_ulLength, buf, ulLength + 1);
	m_ulLength += ulLength;

	// terminate string
	m_szBuf[m_ulLength] = CHAR_EOS;

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
	VA_LIST	vaArgs;

	// get arguments
	VA_START(vaArgs, format);

	AppendFormatVA(format, vaArgs);

	// reset arguments
	VA_END(vaArgs);
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
	VA_LIST vaArgs
	)
{
	GPOS_ASSERT(NULL != format);

	// available space in buffer
	ULONG ulAvailable = m_ulCapacity - m_ulLength;

	// format string
	(void) clib::VsnPrintf(m_szBuf + m_ulLength, ulAvailable, format, vaArgs);

	// terminate string
	m_szBuf[m_ulCapacity - 1] = CHAR_EOS;
	m_ulLength = clib::StrLen(m_szBuf);

	GPOS_ASSERT(m_ulCapacity > m_ulLength);

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

	if (m_ulCapacity - m_ulLength < ulLengthEntry)
	{
		ulLengthEntry = m_ulCapacity - m_ulLength - 1;
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
			m_szBuf[m_ulLength] = CHAR(wsz[i]);
		}
		else
		{
			// substitute wide character
			m_szBuf[m_ulLength] = GPOS_WCHAR_UNPRINTABLE;
		}
		m_ulLength++;
	}

	m_szBuf[m_ulLength] = CHAR_EOS;
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
	m_ulLength = 0;
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
	return (m_ulLength == clib::StrLen(m_szBuf));
}

#endif // GPOS_DEBUG


// EOF

