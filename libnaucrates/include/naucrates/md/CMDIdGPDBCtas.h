//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CMDIdGPDBCtas.h
//
//	@doc:
//		Class for representing mdids for GPDB CTAS queries
//---------------------------------------------------------------------------



#ifndef GPMD_CMDIdGPDBCTAS_H
#define GPMD_CMDIdGPDBCTAS_H

#include "gpos/base.h"

#include "naucrates/md/CMDIdGPDB.h"

namespace gpmd
{
	using namespace gpos;


	//---------------------------------------------------------------------------
	//	@class:
	//		CMDIdGPDBCtas
	//
	//	@doc:
	//		Class for representing ids of GPDB CTAS metadata objects
	//
	//---------------------------------------------------------------------------
	class CMDIdGPDBCtas : public CMDIdGPDB
	{
			
		public:
			// ctor
			explicit 
			CMDIdGPDBCtas(OID oid);
			
			// copy ctor
			explicit
			CMDIdGPDBCtas(const CMDIdGPDBCtas &mdidSource);

			// mdid type
			virtual
			EMDIdType Emdidt() const
			{
				return EmdidGPDBCtas;
			}
			
			// source system id
			virtual
			CSystemId Sysid() const
			{
				return m_sysid;
			}

			// equality check
			virtual
			BOOL Equals(const IMDId *mdid) const;
			
			// is the mdid valid
			virtual
			BOOL IsValid() const;
						
			// debug print of the metadata id
			virtual
			IOstream &OsPrint(IOstream &os) const;
			
			// invalid mdid
			static 
			CMDIdGPDBCtas m_mdidInvalidKey;
			
			// const converter
			static
			const CMDIdGPDBCtas *PmdidConvert(const IMDId *mdid)
			{
				GPOS_ASSERT(NULL != mdid && EmdidGPDBCtas == mdid->Emdidt());

				return dynamic_cast<const CMDIdGPDBCtas *>(mdid);
			}
			
			// non-const converter
			static
			CMDIdGPDBCtas *PmdidConvert(IMDId *mdid)
			{
				GPOS_ASSERT(NULL != mdid && EmdidGPDBCtas == mdid->Emdidt());

				return dynamic_cast<CMDIdGPDBCtas *>(mdid);
			}
	};

}

#endif // !GPMD_CMDIdGPDBCTAS_H

// EOF
