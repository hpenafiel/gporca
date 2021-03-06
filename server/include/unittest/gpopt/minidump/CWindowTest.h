//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2016 Pivotal Software
//
//	@filename:
//		CWindowTest.h
//
//	@doc:
//		Test for Window functions
//---------------------------------------------------------------------------
#ifndef GPOPT_CWindowTest_H
#define GPOPT_CWindowTest_H

#include "gpos/base.h"

namespace gpopt
{
	class CWindowTest
	{
		public:

			// unittests
			static 
			GPOS_RESULT EresUnittest();
	}; // class CWindowTest
}

#endif // !GPOPT_CWindowTest_H

// EOF

