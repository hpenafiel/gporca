//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CColumnDescriptor.cpp
//
//	@doc:
//		Column abstraction for tables, functions, external tables etc.
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpopt/metadata/CColumnDescriptor.h"
#include "naucrates/md/CMDIdGPDB.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CColumnDescriptor::CColumnDescriptor
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CColumnDescriptor::CColumnDescriptor
	(
	IMemoryPool *memory_pool,
	const IMDType *pmdtype,
	INT type_modifier,
	const CName &name,
	INT attno,
	BOOL fNullable,
	ULONG ulWidth
	)
	:
	m_pmdtype(pmdtype),
	m_type_modifier(type_modifier),
	m_name(memory_pool, name),
	m_iAttno(attno),
	m_is_nullable(fNullable),
	m_width(ulWidth)
{
	GPOS_ASSERT(NULL != pmdtype);
	GPOS_ASSERT(pmdtype->MDId()->IsValid());

	if (m_pmdtype->FFixedLength())
	{
		ulWidth = m_pmdtype->Length();
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CColumnDescriptor::~CColumnDescriptor
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CColumnDescriptor::~CColumnDescriptor()
{}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CColumnDescriptor::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CColumnDescriptor::OsPrint
	(
	IOstream &os
	)
	const
{
	return m_name.OsPrint(os);
}

#endif // GPOS_DEBUG

// EOF

