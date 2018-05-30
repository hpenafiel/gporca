//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDatumBoolGPDB.h
//
//	@doc:
//		GPDB-specific bool representation
//---------------------------------------------------------------------------
#ifndef GPNAUCRATES_CDatumBoolGPDB_H
#define GPNAUCRATES_CDatumBoolGPDB_H

#include "gpos/base.h"
#include "naucrates/base/IDatumBool.h"

#include "naucrates/md/CMDTypeBoolGPDB.h"

namespace gpnaucrates
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDatumBoolGPDB
	//
	//	@doc:
	//		GPDB-specific bool representation
	//
	//---------------------------------------------------------------------------
class CDatumBoolGPDB : public IDatumBool
{

	private:

		// type information
		IMDId *m_pmdid;
	
		// boolean value
		BOOL m_value;

		// is null
		BOOL m_is_null;

		// private copy ctor
		CDatumBoolGPDB(const CDatumBoolGPDB &);

	public:

		// ctors
		CDatumBoolGPDB(CSystemId sysid, BOOL value, BOOL is_null = false);
		CDatumBoolGPDB(IMDId *pmdid, BOOL value, BOOL is_null = false);
		
		// dtor
		virtual
		~CDatumBoolGPDB();

		// accessor of metadata type mdid
		virtual
		IMDId *MDId() const;

		// accessor of boolean value
		virtual
		BOOL FValue() const;

		// accessor of size
		virtual
		ULONG UlSize() const;

		// accessor of is null
		virtual
		BOOL IsNull() const;

		// return string representation
		virtual
		const CWStringConst *Pstr(IMemoryPool *memory_pool) const;

		// hash function
		virtual
		ULONG HashValue() const;

		// match function for datums
		virtual
		BOOL FMatch(const IDatum *) const;

		// copy datum
		virtual
		IDatum *PdatumCopy(IMemoryPool *memory_pool) const;
		
		// print function
		virtual
		IOstream &OsPrint(IOstream &os) const;

	}; // class CDatumBoolGPDB
}

#endif // !GPNAUCRATES_CDatumBoolGPDB_H

// EOF
