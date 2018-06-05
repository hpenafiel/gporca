//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDTriggerGPDB.h
//
//	@doc:
//		Implementation of GPDB-specific triggers in the metadata cache
//---------------------------------------------------------------------------

#ifndef GPMD_CMDTriggerGPDB_H
#define GPMD_CMDTriggerGPDB_H

#include "gpos/base.h"
#include "naucrates/md/IMDTrigger.h"

#define GPMD_TRIGGER_ROW 1
#define GPMD_TRIGGER_BEFORE 2
#define GPMD_TRIGGER_INSERT 4
#define GPMD_TRIGGER_DELETE 8
#define GPMD_TRIGGER_UPDATE 16

namespace gpmd
{

	using namespace gpdxl;

	//---------------------------------------------------------------------------
	//	@class:
	//		CMDTriggerGPDB
	//
	//	@doc:
	//		Implementation for GPDB-specific triggers in the metadata cache
	//
	//---------------------------------------------------------------------------
	class CMDTriggerGPDB : public IMDTrigger
	{
		private:
			// memory pool
			IMemoryPool *m_memory_pool;

			// DXL for object
			const CWStringDynamic *m_pstr;

			// trigger id
			IMDId *m_mdid;

			// trigger name
			CMDName *m_mdname;

			// relation id
			IMDId *m_pmdidRel;

			// function id
			IMDId *m_func_mdid;

			// trigger type
			INT m_iType;

			// is trigger enabled
			BOOL m_fEnabled;

			// private copy ctor
			CMDTriggerGPDB(const CMDTriggerGPDB &);

		public:
			// ctor
			CMDTriggerGPDB
				(
				IMemoryPool *memory_pool,
				IMDId *pmdid,
				CMDName *mdname,
				IMDId *pmdidRel,
				IMDId *pmdidFunc,
				INT iType,
				BOOL fEnabled
				);

			// dtor
			~CMDTriggerGPDB();

			// accessors
			virtual
			const CWStringDynamic *Pstr() const
			{
				return m_pstr;
			}

			// trigger id
			virtual
			IMDId *MDId() const
			{
				return m_mdid;
			}

			// trigger name
			virtual
			CMDName Mdname() const
			{
				return *m_mdname;
			}

			// relation mdid
			virtual
			IMDId *PmdidRel() const
			{
				return m_pmdidRel;
			}

			// function mdid
			virtual
			IMDId *FuncMdId() const
			{
				return m_func_mdid;
			}

			// does trigger execute on a row-level
			virtual
			BOOL FRow() const;

			// is this a before trigger
			virtual
			BOOL FBefore() const;

			// is this an insert trigger
			virtual
			BOOL Insert() const;

			// is this a delete trigger
			virtual
			BOOL FDelete() const;

			// is this an update trigger
			virtual
			BOOL FUpdate() const;

			// is trigger enabled
			virtual
			BOOL FEnabled() const
			{
				return m_fEnabled;
			}

			// serialize object in DXL format
			virtual
			void Serialize(gpdxl::CXMLSerializer *xml_serializer) const;

#ifdef GPOS_DEBUG
			// debug print of the type in the provided stream
			virtual
			void DebugPrint(IOstream &os) const;
#endif
	};
}

#endif // !GPMD_CMDTriggerGPDB_H

// EOF
