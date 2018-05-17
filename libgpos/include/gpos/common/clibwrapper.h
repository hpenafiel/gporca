//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (c) 2004-2015 Pivotal Software, Inc.
//
//	@filename:
//	       	clibwrapper.h
//
//	@doc:
//	       	Wrapper for functions in C library
//
//---------------------------------------------------------------------------

#ifndef GPOS_clibwrapper_H
#define GPOS_clibwrapper_H

#define VA_START(vaList, last)  va_start(vaList, last);
#define VA_END(vaList)		va_end(vaList)
#define VA_ARG(vaList, type)    va_arg(vaList,type)

#include <unistd.h>
#include "gpos/types.h"
#include "gpos/common/clibtypes.h"

namespace gpos
{
	namespace clib
	{

		typedef INT (*Comparator)(const void *, const void *);

#ifdef GPOS_sparc

#include <ucontext.h>

		typedef INT (*Callback)(ULONG_PTR, INT, void *);

		// get current user context
		INT GetContext(ucontext_t *user_ctxt);

		// call the user-supplied function callback for each routine found on
		// the call stack and each signal handler invoked
		INT WalkContext(const ucontext_t *user_ctxt, Callback callback, void *arg);

#endif

		// get an environment variable
		CHAR *GetEnv(const CHAR *name);

		// compare a specified number of bytes of two regions of memory
		INT MemCmp(const void *left, const void *right, SIZE_T num_bytes);

		 // sleep given number of microseconds
		void USleep(ULONG usecs);

		// compare two strings
		INT StrCmp(const CHAR *left, const CHAR *right);

		// compare two strings up to a specified number of characters
		INT StrNCmp(const CHAR *left, const CHAR *right, SIZE_T num_bytes);

		// compare two strings up to a specified number of wide characters
		INT WcStrNCmp(const WCHAR *left, const WCHAR *right, SIZE_T num_bytes);

		// copy two strings up to a specified number of wide characters
		WCHAR *WcStrNCpy(WCHAR *dest, const WCHAR *src, SIZE_T num_bytes);

		// copy a specified number of bytes between two memory areas
		void* MemCpy(void *dest, const void* src, SIZE_T num_bytes);

		// copy a specified number of wide characters
		WCHAR *WcMemCpy(WCHAR *dest, const WCHAR *src, SIZE_T num_bytes);

		// copy a specified number of characters
		CHAR *StrNCpy(CHAR *dest, const CHAR *src, SIZE_T num_bytes);

		// find the first occurrence of the character c in src
		CHAR *StrChr(const CHAR *src, INT c);

		// set a specified number of bytes to a specified value
		void* MemSet(void *dest, INT value, SIZE_T num_bytes);

		// calculate the length of a wide-character string
		ULONG WcStrLen(const WCHAR *dest);

		// calculate the length of a string
		ULONG StrLen(const CHAR *buf);

		// sort a specified number of elements
		void QSort(void *dest, SIZE_T num_bytes, SIZE_T size,  Comparator fnComparator);

		// parse command-line options
		INT GetOpt(INT argc, CHAR * const argv[], const CHAR *opt_string);

		// convert string to long integer
		LINT StrToL(const CHAR *val, CHAR **end, ULONG base);

		// convert string to long long integer
		LINT StrToLL(const CHAR *val, CHAR **end, ULONG base);

		// convert string to double
		DOUBLE StrToD(const CHAR *str);

		// return a pseudo-random integer between 0 and RAND_MAX
		ULONG Rand(ULONG *seed);

		// format wide character output conversion
		INT VswPrintf(WCHAR *wcstr, SIZE_T max_len, const WCHAR * format, VA_LIST vaArgs);

		// format string
		INT VsnPrintf(CHAR *src, SIZE_T size, const CHAR *format, VA_LIST vaArgs);

		// return string describing error number
		void StrErrorR(INT errnum, CHAR *buf, SIZE_T buf_len);

		// convert the calendar time time to broken-time representation
		TIME *LocalTimeR(const TIME_T *time, TIME *result);

		// allocate dynamic memory
		void *Malloc(SIZE_T size);

		// free dynamic memory
		void Free(void *src);

		// convert a wide character to a multibyte sequence
		INT WcToMb(CHAR *dest, WCHAR src);

		// convert a wide-character string to a multi-byte string
		LINT WcsToMbs(CHAR *dest, WCHAR *src, ULONG_PTR dest_size);

		// convert a multibyte sequence to wide character array
		ULONG MbToWcs(WCHAR *dest, const CHAR *src, SIZE_T len);

		// return a pointer to the start of the NULL-terminated symbol
		CHAR *Demangle(const CHAR *symbol, CHAR *buf, SIZE_T *len, INT *status);

		// resolve symbol information from its address
		void DlAddr(void *addr, DL_INFO *info);

	} //namespace clib
}

#endif // !GPOS_clibwrapper_H

// EOF
