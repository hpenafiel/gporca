//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDIdColStats.h
//
//	@doc:
//		Class for representing mdids for column statistics
//---------------------------------------------------------------------------



#ifndef GPMD_CMDIdColStats_H
#define GPMD_CMDIdColStats_H

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
	//		CMDIdColStats
	//
	//	@doc:
	//		Class for representing ids of column stats objects
	//
	//---------------------------------------------------------------------------
	class CMDIdColStats : public IMDId
	{
		private:
			// mdid of base relation
			CMDIdGPDB *m_rel_mdid;
			
			// position of the attribute in the base relation
			ULONG m_ulPos;
			
			// buffer for the serialized mdid
			WCHAR m_wszBuffer[GPDXL_MDID_LENGTH];
			
			// string representation of the mdid
			CWStringStatic m_str;
			
			// private copy ctor
			CMDIdColStats(const CMDIdColStats &);
			
			// serialize mdid
			void Serialize();
			
		public:
			// ctor
			CMDIdColStats(CMDIdGPDB *rel_mdid, ULONG attno);
			
			// dtor
			virtual
			~CMDIdColStats();
			
			virtual
			EMDIdType Emdidt() const
			{
				return EmdidColStats;
			}
			
			// string representation of mdid
			virtual
			const WCHAR *GetBuffer() const;
			
			// source system id
			virtual
			CSystemId Sysid() const
			{
				return m_rel_mdid->Sysid();
			}
			
			// accessors
			IMDId *GetRelMdId() const;
			ULONG UlPos() const;

			// equality check
			virtual
			BOOL Equals(const IMDId *mdid) const;
			
			// computes the hash value for the metadata id
			virtual
			ULONG HashValue() const
			{
				return gpos::CombineHashes(m_rel_mdid->HashValue(),
											gpos::HashValue(&m_ulPos));
			}
			
			// is the mdid valid
			virtual
			BOOL IsValid() const
			{
				return IMDId::IsValid(m_rel_mdid);
			}

			// serialize mdid in DXL as the value of the specified attribute 
			virtual
			void Serialize(CXMLSerializer *xml_serializer, const CWStringConst *pstrAttribute) const;
						
			// debug print of the metadata id
			virtual
			IOstream &OsPrint(IOstream &os) const;
			
			// const converter
			static
			const CMDIdColStats *PmdidConvert(const IMDId *mdid)
			{
				GPOS_ASSERT(NULL != mdid && EmdidColStats == mdid->Emdidt());

				return dynamic_cast<const CMDIdColStats *>(mdid);
			}
			
			// non-const converter
			static
			CMDIdColStats *PmdidConvert(IMDId *mdid)
			{
				GPOS_ASSERT(NULL != mdid && EmdidColStats == mdid->Emdidt());

				return dynamic_cast<CMDIdColStats *>(mdid);
			}

	};

}



#endif // !GPMD_CMDIdColStats_H

// EOF
