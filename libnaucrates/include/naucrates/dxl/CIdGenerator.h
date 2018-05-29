//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CIdGenerator.h
//
//	@doc:
//		Class providing methods for a ULONG counter
//
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPDXL_CIdGenerator_H
#define GPDXL_CIdGenerator_H

#define GPDXL_INVALID_ID ULONG_MAX

#include "gpos/base.h"

namespace gpdxl
{
	using namespace gpos;

	class CIdGenerator {
		private:
			ULONG id;
		public:
			explicit CIdGenerator(ULONG);
			ULONG next_id();
			ULONG current_id();
	};
}
#endif // GPDXL_CIdGenerator_H

// EOF
