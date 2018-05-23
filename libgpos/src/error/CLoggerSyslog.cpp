//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CLoggerSyslog.cpp
//
//	@doc:
//		Implementation of Syslog logging
//---------------------------------------------------------------------------

#include <syslog.h>

#include "gpos/common/syslibwrapper.h"
#include "gpos/error/CLoggerSyslog.h"
#include "gpos/string/CStringStatic.h"

using namespace gpos;

// initialization of static members
CLoggerSyslog CLoggerSyslog::m_loggerAlert
	(
	NULL /*szName*/,
#ifndef GPOS_SunOS
	LOG_PERROR |
#endif // GPOS_SunOS
	LOG_CONS,
	LOG_ALERT
	)
	;


//---------------------------------------------------------------------------
//	@function:
//		CLoggerSyslog::CLoggerSyslog
//
//	@doc:
//		Ctor - set executable name, initialization flags and message priority
//
//---------------------------------------------------------------------------
CLoggerSyslog::CLoggerSyslog
	(
	const CHAR *szProcName,
	ULONG ulInitMask,
	ULONG ulMessagePriority
	)
	:
	m_szProcName(szProcName),
	m_ulInitMask(ulInitMask),
	m_ulMessagePriority(ulMessagePriority)
{}


//---------------------------------------------------------------------------
//	@function:
//		CLoggerSyslog::~CLoggerSyslog
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CLoggerSyslog::~CLoggerSyslog()
{}

//---------------------------------------------------------------------------
//	@function:
//		CLoggerSyslog::Write
//
//	@doc:
//		Write string to syslog
//
//---------------------------------------------------------------------------
void
CLoggerSyslog::Write
	(
	const WCHAR *log_entry,
	ULONG // severity
	)
{
	CHAR *szBuffer = CLogger::Msg();

	// create message
	CStringStatic str(szBuffer, GPOS_LOG_MESSAGE_BUFFER_SIZE);
	str.AppendConvert(log_entry);

	// send message to syslog
	syslib::OpenLog(m_szProcName, m_ulInitMask, LOG_USER);
	syslib::SysLog(m_ulMessagePriority, szBuffer);
	syslib::CloseLog();
}


//---------------------------------------------------------------------------
//	@function:
//		CLoggerSyslog::Write
//
//	@doc:
//		Write alert message to syslog - use ASCII characters only
//
//---------------------------------------------------------------------------
void
CLoggerSyslog::Alert
	(
	const WCHAR *msg
	)
{
	m_loggerAlert.Write(msg, CException::ExsevError);
}

// EOF

