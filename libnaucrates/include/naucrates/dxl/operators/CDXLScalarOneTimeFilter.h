//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarOneTimeFilter.h
//
//	@doc:
//		Class for representing a scalar filter that is executed once inside DXL physical operators.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarOneTimeFilter_H
#define GPDXL_CDXLScalarOneTimeFilter_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalarFilter.h"


namespace gpdxl
{
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarOneTimeFilter
	//
	//	@doc:
	//		Class for representing DXL filter operators
	//
	//---------------------------------------------------------------------------
	class CDXLScalarOneTimeFilter : public CDXLScalarFilter
	{
		private:
			// private copy ctor
		CDXLScalarOneTimeFilter(CDXLScalarOneTimeFilter&);
			
		public:
			// ctor
			explicit
			CDXLScalarOneTimeFilter(IMemoryPool *memory_pool);
			
			// accessors
			Edxlopid Edxlop() const;
			const CWStringConst *PstrOpName() const;
			
			// conversion function
			static
			CDXLScalarOneTimeFilter *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarOneTimeFilter == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarOneTimeFilter*>(dxl_op);
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *, const CDXLNode *) const;

			// does the operator return a boolean result
			virtual
			BOOL FBoolean
					(
					CMDAccessor *//pmda
					)
					const
			{
				GPOS_ASSERT(!"Invalid function call for a container operator");
				return false;
			}
	};
}
#endif // !GPDXL_CDXLScalarOneTimeFilter_H

// EOF

