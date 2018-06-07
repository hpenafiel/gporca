//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDIdCast.h
//
//	@doc:
//		Class for representing mdids of cast functions
//---------------------------------------------------------------------------

#ifndef GPMD_CMDIdCastFunc_H
#define GPMD_CMDIdCastFunc_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/string/CWStringConst.h"

#include "naucrates/dxl/gpdb_types.h"

#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CSystemId.h"

namespace gpmd
{
	using namespace gpos;


	//---------------------------------------------------------------------------
	//	@class:
	//		CMDIdCast
	//
	//	@doc:
	//		Class for representing ids of cast objects
	//
	//---------------------------------------------------------------------------
	class CMDIdCast : public IMDId
	{
		private:
			// mdid of source type
			CMDIdGPDB *m_pmdidSrc;
			
			// mdid of destinatin type
			CMDIdGPDB *m_pmdidDest;

			
			// buffer for the serialized mdid
			WCHAR m_wszBuffer[GPDXL_MDID_LENGTH];
			
			// string representation of the mdid
			CWStringStatic m_str;
			
			// private copy ctor
			CMDIdCast(const CMDIdCast &);
			
			// serialize mdid
			void Serialize();
			
		public:
			// ctor
			CMDIdCast(CMDIdGPDB *mdid_src, CMDIdGPDB *mdid_dest);
			
			// dtor
			virtual
			~CMDIdCast();
			
			virtual
			EMDIdType Emdidt() const
			{
				return EmdidCastFunc;
			}
			
			// string representation of mdid
			virtual
			const WCHAR *GetBuffer() const;
			
			// source system id
			virtual
			CSystemId Sysid() const
			{
				return m_pmdidSrc->Sysid();
			}
			
			// source type id
			IMDId *PmdidSrc() const;

			// destination type id
			IMDId *PmdidDest() const;
			
			// equality check
			virtual
			BOOL Equals(const IMDId *pmdid) const;
			
			// computes the hash value for the metadata id
			virtual
			ULONG HashValue() const
			{
				return gpos::CombineHashes
							(
							Emdidt(), 
							gpos::CombineHashes(m_pmdidSrc->HashValue(), m_pmdidDest->HashValue())
							);							
			}
			
			// is the mdid valid
			virtual
			BOOL IsValid() const
			{
				return IMDId::IsValid(m_pmdidSrc) && IMDId::IsValid(m_pmdidDest);
			}

			// serialize mdid in DXL as the value of the specified attribute 
			virtual
			void Serialize(CXMLSerializer *xml_serializer, const CWStringConst *pstrAttribute) const;
						
			// debug print of the metadata id
			virtual
			IOstream &OsPrint(IOstream &os) const;
			
			// const converter
			static
			const CMDIdCast *PmdidConvert(const IMDId *pmdid)
			{
				GPOS_ASSERT(NULL != pmdid && EmdidCastFunc == pmdid->Emdidt());

				return dynamic_cast<const CMDIdCast *>(pmdid);
			}
			
			// non-const converter
			static
			CMDIdCast *PmdidConvert(IMDId *pmdid)
			{
				GPOS_ASSERT(NULL != pmdid && EmdidCastFunc == pmdid->Emdidt());

				return dynamic_cast<CMDIdCast *>(pmdid);
			}
	};
}

#endif // !GPMD_CMDIdCastFunc_H

// EOF
