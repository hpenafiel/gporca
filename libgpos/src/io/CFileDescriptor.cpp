//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CFileDescriptor.cpp
//
//	@doc:
//		File descriptor abstraction
//---------------------------------------------------------------------------


#include "gpos/base.h"
#include "gpos/io/ioutils.h"
#include "gpos/io/CFileDescriptor.h"
#include "gpos/string/CStringStatic.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CFileDescriptor::CFileDescriptor
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CFileDescriptor::CFileDescriptor()
	:
	m_iFileDescr(GPOS_FILE_DESCR_INVALID)
{}


//---------------------------------------------------------------------------
//	@function:
//		CFileDescriptor::OpenInternal
//
//	@doc:
//		Open file descriptor
//
//---------------------------------------------------------------------------
void
CFileDescriptor::OpenInternal
	(
	const CHAR *file_path,
	ULONG ulMode,
	ULONG permission_bits
	)
{
	GPOS_ASSERT(!FOpened());

	BOOL fOpened = false;

	while (!fOpened)
	{
		m_iFileDescr = GPOS_FILE_DESCR_INVALID;

		// create file with given mode and permissions and check to simulate I/O error
		GPOS_CHECK_SIM_IO_ERR(&m_iFileDescr, ioutils::OpenFile(file_path, ulMode, permission_bits));

		// check for error
		if (GPOS_FILE_DESCR_INVALID == m_iFileDescr)
		{
			// in case an interrupt was received we retry
			if (EINTR == errno)
			{
				GPOS_CHECK_ABORT;

				continue;
			}

			GPOS_RAISE(CException::ExmaSystem, CException::ExmiIOError, errno);
		}

		fOpened = true;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CFile::~CFile
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CFileDescriptor::~CFileDescriptor()
{
	if (FOpened())
	{
		CloseInternal();
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CFile::CloseInternal
//
//	@doc:
//		Close file
//
//---------------------------------------------------------------------------
void
CFileDescriptor::CloseInternal()
{
	GPOS_ASSERT(FOpened());

	BOOL fClosed = false;

	while (!fClosed)
	{
		INT res = ioutils::CloseFile(m_iFileDescr);

		// check for error
		if (0 != res)
		{
			GPOS_ASSERT(EINTR == errno || EIO == errno);

			// in case an interrupt was received we retry
			if (EINTR == errno)
			{
				continue;
			}
		}

		fClosed = true;
	}

	m_iFileDescr = GPOS_FILE_DESCR_INVALID;
}

// EOF

