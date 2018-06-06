//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDPartConstraintGPDB.h
//
//	@doc:
//		Class representing a GPDB partition constraint in the MD cache
//---------------------------------------------------------------------------

#ifndef GPMD_CMDPartConstraintGPDB_H
#define GPMD_CMDPartConstraintGPDB_H

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDPartConstraint.h"
#include "naucrates/md/CMDName.h"

// fwd decl
namespace gpdxl
{
	class CXMLSerializer;
}

namespace gpopt
{
	class CExpression;
	class CMDAccessor;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;

	//---------------------------------------------------------------------------
	//	@class:
	//		CMDPartConstraintGPDB
	//
	//	@doc:
	//		Class representing a GPDB partition constraint in the MD cache
	//
	//---------------------------------------------------------------------------
	class CMDPartConstraintGPDB : public IMDPartConstraint
	{
		private:

			// memory pool
			IMemoryPool *m_memory_pool;

			// included default partitions
			ULongPtrArray *m_pdrgpulDefaultParts;
			
			// is constraint unbounded
			BOOL m_fUnbounded;

			// the DXL representation of the part constraint
			CDXLNode *m_dxl_node;
		public:

			// ctor
			CMDPartConstraintGPDB(IMemoryPool *memory_pool, ULongPtrArray *pdrgpulDefaultParts, BOOL fUnbounded, CDXLNode *dxlnode);

			// dtor
			virtual
			~CMDPartConstraintGPDB();

			// serialize constraint in DXL format
			virtual
			void Serialize(CXMLSerializer *xml_serializer) const;
			
			// the scalar expression of the check constraint
			virtual
			CExpression *Pexpr(IMemoryPool *memory_pool, CMDAccessor *md_accessor, DrgPcr *pdrgpcr) const;
			
			// included default partitions
			virtual
			ULongPtrArray *PdrgpulDefaultParts() const;

			// is constraint unbounded
			virtual
			BOOL FUnbounded() const;

	};
}

#endif // !GPMD_CMDPartConstraintGPDB_H

// EOF

