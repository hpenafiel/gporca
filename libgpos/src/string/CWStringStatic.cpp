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
	WCHAR wszBuffer[],
	ULONG ulCapacity
	)
	:
	CWString
		(
		0 // length
		),
	m_ulCapacity(ulCapacity)
{
	GPOS_ASSERT(NULL != wszBuffer);
	GPOS_ASSERT(0 < m_ulCapacity);

	m_wszBuf = wszBuffer;
	m_wszBuf[0] = WCHAR_EOS;
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
	WCHAR wszBuffer[],
	ULONG ulCapacity,
	const WCHAR wszInit[]
	)
	:
	CWString
		(
		0 // length
		),
	m_ulCapacity(ulCapacity)
{
	GPOS_ASSERT(NULL != wszBuffer);
	GPOS_ASSERT(0 < m_ulCapacity);

	m_wszBuf = wszBuffer;
	AppendBuffer(wszInit);
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
	if (0 == length || m_ulCapacity == m_length)
	{
		return;
	}
	
	// check if new length exceeds capacity
	if (m_ulCapacity <= length + m_length)
	{
		// truncate string
		length = m_ulCapacity - m_length - 1;
	}

	GPOS_ASSERT(m_ulCapacity > length + m_length);

	clib::WcStrNCpy(m_wszBuf + m_length, wstrbuf, length + 1);
	m_length += length;
	
	// terminate string
	m_wszBuf[m_length] = WCHAR_EOS;

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
	const WCHAR *wsz
	)
{
	AppendBuffer(wsz);
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
	const CHAR *sz
	)
{
	GPOS_ASSERT(NULL != sz);
	if (0 ==  GPOS_SZ_LENGTH(sz) || m_ulCapacity == m_length)
	{
		return;
	}

	ULONG length = GPOS_SZ_LENGTH(sz);
	if (length >= GPOS_STATIC_STR_BUFFER_LENGTH)
	{
		// if input string is larger than buffer length, use AppendFormat
		AppendFormat(GPOS_WSZ_LIT("%s"), sz);
		return;
	}

	// otherwise, append to wide string character array directly
	WCHAR wstrbuf[GPOS_STATIC_STR_BUFFER_LENGTH];

	// convert input string to wide character buffer
	#ifdef GPOS_DEBUG
	ULONG ulLen =
	#endif // GPOS_DEBUG
		clib::MbToWcs(wstrbuf, sz, length);
	GPOS_ASSERT(ulLen == length);

	// check if new length exceeds capacity
	if (m_ulCapacity <= length + m_length)
	{
		// truncate string
		length = m_ulCapacity - m_length - 1;
	}
	GPOS_ASSERT(m_ulCapacity > length + m_length);

	// append input string to current end of buffer
	(void) clib::WcMemCpy(m_wszBuf + m_length, wstrbuf, length + 1);

	m_length += length;
	m_wszBuf[m_length] = WCHAR_EOS;

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
	VA_LIST	vaArgs;

	// get arguments
	VA_START(vaArgs, format);

	AppendFormatVA(format, vaArgs);

	// reset arguments
	VA_END(vaArgs);
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
	VA_LIST vaArgs
	)
{
	GPOS_ASSERT(NULL != format);

	// available space in buffer
	ULONG ulAvailable = m_ulCapacity - m_length;

	// format string
	(void) clib::VswPrintf(m_wszBuf + m_length, ulAvailable, format, vaArgs);

	m_wszBuf[m_ulCapacity - 1] = WCHAR_EOS;
	m_length = GPOS_WSZ_LENGTH(m_wszBuf);

	GPOS_ASSERT(m_ulCapacity > m_length);

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
	m_wszBuf[0] = WCHAR_EOS;
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
	const CWStringBase *pstr,
	WCHAR wc,
	const WCHAR *wszReplace
	)
{
	GPOS_ASSERT(NULL != pstr);

	if (pstr->IsEmpty())
	{
		return;
	}

	ULONG length = pstr->Length();
	ULONG ulLengthReplace =  GPOS_WSZ_LENGTH(wszReplace);
	ULONG ulLengthNew = m_length;
	const WCHAR *wsz = pstr->GetBuffer();

	for (ULONG i = 0; i < length && ulLengthNew < m_ulCapacity - 1; i++)
	{
		if (wc == wsz[i] && !pstr->HasEscapedCharAt(i))
		{
			// check for overflow
			ULONG ulLengthCopy = std::min(ulLengthReplace, m_ulCapacity - ulLengthNew - 1);

			clib::WcStrNCpy(m_wszBuf + ulLengthNew, wszReplace, ulLengthCopy);
			ulLengthNew += ulLengthCopy;
		}
		else
		{
			m_wszBuf[ulLengthNew] = wsz[i];
			ulLengthNew++;
		}

		GPOS_ASSERT(ulLengthNew < m_ulCapacity);
	}

	// terminate string
	m_wszBuf[ulLengthNew] = WCHAR_EOS;

	m_length = ulLengthNew;
	GPOS_ASSERT(IsValid());
}


// EOF

