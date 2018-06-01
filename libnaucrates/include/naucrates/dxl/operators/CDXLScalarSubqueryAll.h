//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryAll.h
//
//	@doc:
//		Class for representing ALL subqueries
//---------------------------------------------------------------------------



#ifndef GPDXL_CDXLScalarSubqueryAll_H
#define GPDXL_CDXLScalarSubqueryAll_H

#include "gpos/base.h"

#include "naucrates/md/IMDId.h"
#include "naucrates/md/CMDName.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLScalarSubqueryQuantified.h"

namespace gpdxl
{

	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarSubqueryAll
	//
	//	@doc:
	//		Class for representing ALL subqueries
	//
	//---------------------------------------------------------------------------
	class CDXLScalarSubqueryAll : public CDXLScalarSubqueryQuantified
	{			

		private:
		
			// private copy ctor
			CDXLScalarSubqueryAll(CDXLScalarSubqueryAll&);
			
		public:
			// ctor
			CDXLScalarSubqueryAll(IMemoryPool *memory_pool, IMDId *pmdidScalarOp, CMDName *mdname, ULONG ulColId);

			// ident accessors
			Edxlopid Edxlop() const;
			
			// name of the operator
			const CWStringConst *PstrOpName() const;
			
			// conversion function
			static
			CDXLScalarSubqueryAll *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarSubqueryAll == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarSubqueryAll*>(dxl_op);
			}

			// does the operator return a boolean result
			virtual
			BOOL FBoolean
					(
					CMDAccessor *//pmda
					)
					const
			{
				return true;
			}
	};
}


#endif // !GPDXL_CDXLScalarSubqueryAll_H

// EOF
