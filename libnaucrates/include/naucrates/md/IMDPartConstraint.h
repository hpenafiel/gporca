//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		IMDPartConstraint.h
//
//	@doc:
//		Interface class for partition constraints in the MD cache
//---------------------------------------------------------------------------



#ifndef GPMD_IMDPartConstraint_H
#define GPMD_IMDPartConstraint_H

#include "gpos/base.h"

#include "naucrates/md/IMDInterface.h"
#include "gpopt/base/CColRef.h"

// fwd decl
namespace gpopt
{
	class CExpression;
	class CMDAccessor;
}

namespace gpmd
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		IMDPartConstraint
	//
	//	@doc:
	//		Interface class for partition constraints in the MD cache
	//
	//---------------------------------------------------------------------------
	class IMDPartConstraint : public IMDInterface
	{		
		public:
			
			// extract the scalar expression of the constraint with the given
			// column mappings
			virtual
			CExpression *Pexpr(IMemoryPool *memory_pool, CMDAccessor *md_accessor, DrgPcr *pdrgpcr) const = 0;
			
			// included default partitions
			virtual
			ULongPtrArray *PdrgpulDefaultParts() const = 0;

			// is constraint unbounded
			virtual
			BOOL FUnbounded() const = 0;
			
			// serialize constraint in DXL format
			virtual
			void Serialize(CXMLSerializer *xml_serializer) const = 0;

	};
}



#endif // !GPMD_IMDPartConstraint_H

// EOF
