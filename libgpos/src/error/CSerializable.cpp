//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CSerializable.cpp
//
//	@doc:
//		Interface for serializable objects
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/error/CErrorContext.h"
#include "gpos/error/CSerializable.h"
#include "gpos/task/CTask.h"

using namespace gpos;


//---------------------------------------------------------------------------
//	@function:
//		CSerializable::CSerializable
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CSerializable::CSerializable()
{
	CTask *ptsk = CTask::TaskSelf();

	GPOS_ASSERT(NULL != ptsk);

	ptsk->ErrCtxtConvert()->Register(this);
}


//---------------------------------------------------------------------------
//	@function:
//		CSerializable::~CSerializable
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CSerializable::~CSerializable()
{
	CTask *ptsk = CTask::TaskSelf();

	GPOS_ASSERT(NULL != ptsk);

	ptsk->ErrCtxtConvert()->Unregister(this);
}


// EOF

