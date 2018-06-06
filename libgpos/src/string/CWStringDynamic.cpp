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
	IMemoryPool *memory_pool
	)
	:
	CWString
		(
		0 // length
		),
	m_memory_pool(memory_pool),
	m_ulCapacity(0)
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
	IMemoryPool *memory_pool,
	const WCHAR *wstrbuf
	)
	:
	CWString
		(
		GPOS_WSZ_LENGTH(wstrbuf)
		),
	m_memory_pool(memory_pool),
	m_ulCapacity(0)
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
	if (NULL != m_wszBuf && &m_empty_wcstr != m_wszBuf)
	{
		GPOS_DELETE_ARRAY(m_wszBuf);
	}

	m_wszBuf = const_cast<WCHAR *>(&m_empty_wcstr);
	m_length = 0;
	m_ulCapacity = 0;
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
	ULONG ulNewLength = m_length + length;
	if (ulNewLength + 1 > m_ulCapacity)
	{
		IncreaseCapacity(ulNewLength);
	}

	clib::WcStrNCpy(m_wszBuf + m_length, wsz, length + 1);
	m_length = ulNewLength;

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
	const WCHAR *wsz
	)
{
	AppendBuffer(wsz);
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
	const CHAR *sz
	)
{
	GPOS_ASSERT(NULL != sz);

	// expand buffer if needed
	const ULONG length = GPOS_SZ_LENGTH(sz);
	ULONG ulNewLength = m_length + length;
	if (ulNewLength + 1 > m_ulCapacity)
	{
		IncreaseCapacity(ulNewLength);
	}
	WCHAR *wstrbuf = GPOS_NEW_ARRAY(m_memory_pool, WCHAR, length + 1);

	// convert input string to wide character buffer
#ifdef GPOS_DEBUG
	ULONG ulLen =
#endif // GPOS_DEBUG
		clib::MbToWcs(wstrbuf, sz, length);
	GPOS_ASSERT(ulLen == length);

	// append input string to current end of buffer
	(void) clib::WcMemCpy(m_wszBuf + m_length, wstrbuf, length + 1);
	GPOS_DELETE_ARRAY(wstrbuf);

	m_wszBuf[ulNewLength] = WCHAR_EOS;
	m_length = ulNewLength;

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

	VA_LIST	vaArgs;

	// determine length of format string after expansion
	INT res = -1;

	// attempt to fit the formatted string in a static array
	WCHAR wszBufStatic[GPOS_WSTR_DYNAMIC_STATIC_BUFFER];

	// get arguments
	VA_START(vaArgs, format);

	// try expanding the formatted string in the buffer
	res = VswPrintf(wszBufStatic, GPOS_ARRAY_SIZE(wszBufStatic), format, vaArgs);

	// reset arguments
	VA_END(vaArgs);
	GPOS_ASSERT(-1 <= res);

	// estimated number of characters in expanded format string
	ULONG ulSize = std::max(GPOS_WSZ_LENGTH(format), GPOS_ARRAY_SIZE(wszBufStatic));

	// if the static buffer is too small, find the formatted string
	// length by trying to store it in a buffer of increasing size
	while (-1 == res)
	{
		// try with a bigger buffer this time
		ulSize *= 2;
		CAutoRg<WCHAR> a_wszBuf;
		a_wszBuf = GPOS_NEW_ARRAY(m_memory_pool, WCHAR, ulSize + 1);

		// get arguments
		VA_START(vaArgs, format);

		// try expanding the formatted string in the buffer
		res = VswPrintf(a_wszBuf.Rgt(), ulSize, format, vaArgs);

		// reset arguments
		VA_END(vaArgs);

		GPOS_ASSERT(-1 <= res);
	}
	// verify required buffer was not bigger than allowed
	GPOS_ASSERT(res >= 0);

	// expand buffer if needed
	ULONG ulNewLength = m_length + ULONG(res);
	if (ulNewLength + 1 > m_ulCapacity)
	{
		IncreaseCapacity(ulNewLength);
	}

	// get arguments
	VA_START(vaArgs, format);

	// print vaArgs to string
	VswPrintf(m_wszBuf + m_length, res + 1, format, vaArgs);

	// reset arguments
	VA_END(vaArgs);

	m_length = ulNewLength;
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
	const WCHAR *wszReplace
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
	const WCHAR * wsz = str->GetBuffer();

	ULONG ulLengthReplace =  GPOS_WSZ_LENGTH(wszReplace);
	ULONG ulNewLength = m_length + length + (ulLengthReplace - 1) * occurrences;
	if (ulNewLength + 1 > m_ulCapacity)
	{
		IncreaseCapacity(ulNewLength);
	}

	// append new contents while replacing character with escaping string
	for (ULONG i = 0, j = m_length; i < length; i++)
	{
		if (wc == wsz[i] && !str->HasEscapedCharAt(i))
		{
			clib::WcStrNCpy(m_wszBuf + j, wszReplace, ulLengthReplace);
			j += ulLengthReplace;
		}
		else
		{
			m_wszBuf[j++] = wsz[i];
		}
	}

	// terminate string
	m_wszBuf[ulNewLength] = WCHAR_EOS;
	m_length = ulNewLength;

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
	ULONG ulRequested
	)
{
	GPOS_ASSERT(ulRequested + 1 > m_ulCapacity);

	ULONG ulCapacity = UlCapacity(ulRequested + 1);
	GPOS_ASSERT(ulCapacity > ulRequested + 1);
	GPOS_ASSERT(ulCapacity >= (m_ulCapacity << 1));

	CAutoRg<WCHAR> a_wszNewBuf;
	a_wszNewBuf = GPOS_NEW_ARRAY(m_memory_pool, WCHAR, ulCapacity);
	if (0 < m_length)
	{
		// current string is not empty: copy it to the resulting string
		a_wszNewBuf = clib::WcStrNCpy(a_wszNewBuf.Rgt(), m_wszBuf, m_length);
	}

	// release old buffer
	if (m_wszBuf != &m_empty_wcstr)
	{
		GPOS_DELETE_ARRAY(m_wszBuf);
	}
	m_wszBuf = a_wszNewBuf.RgtReset();
	m_ulCapacity = ulCapacity;
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
	ULONG ulRequested
	)
{
	ULONG ulCapacity = GPOS_WSTR_DYNAMIC_CAPACITY_INIT;
	while (ulCapacity <= ulRequested + 1)
	{
		ulCapacity = ulCapacity << 1;
	}

	return ulCapacity;
}


// EOF

