//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2010 Greenplum Inc.
//
//	@filename:
//		COstreamString.cpp
//
//	@doc:
//		Implementation of basic wide character output stream
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringConst.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		COstreamString::COstreamString
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
COstreamString::COstreamString
    (
	CWString *pws
    )
	: 
    COstream(),
    m_string(pws)
{
	GPOS_ASSERT(m_string && "Backing string cannot be NULL");
}

//---------------------------------------------------------------------------
//	@function:
//		COstreamString::operator<<
//
//	@doc:
//		WCHAR array write thru;
//
//---------------------------------------------------------------------------
IOstream&
COstreamString::operator << 
    (
	const WCHAR *input_wchar_array
    )
{
	m_string->AppendWideCharArray(input_wchar_array);

	return *this;
}

//---------------------------------------------------------------------------
//	@function:
//		COstreamString::operator<<
//
//	@doc:
//		CHAR array write thru;
//
//---------------------------------------------------------------------------
IOstream&
COstreamString::operator <<
    (
	const CHAR *input_char
    )
{
	m_string->AppendCharArray(input_char);

	return *this;
}


//---------------------------------------------------------------------------
//	@function:
//		COstreamString::operator<<
//
//	@doc:
//		WCHAR write thru;
//
//---------------------------------------------------------------------------
IOstream&
COstreamString::operator <<
    (
	const WCHAR input_wchar
    )
{
	WCHAR input_wchar_array[2];
	input_wchar_array[0] = input_wchar;
	input_wchar_array[1] = L'\0';
	m_string->AppendWideCharArray(input_wchar_array);

	return *this;
}


//---------------------------------------------------------------------------
//	@function:
//		COstreamString::operator<<
//
//	@doc:
//		CHAR write thru;
//
//---------------------------------------------------------------------------
IOstream&
COstreamString::operator <<
    (
	const CHAR c
    )
{
	CHAR input_char[2];
	input_char[0] = c;
	input_char[1] = '\0';
	m_string->AppendCharArray(input_char);

	return *this;
}


// EOF

