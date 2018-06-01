//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarFilter.h
//
//	@doc:
//		Class for representing a scalar filter node inside DXL physical operators.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarFilter_H
#define GPDXL_CDXLScalarFilter_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"


namespace gpdxl
{
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarFilter
	//
	//	@doc:
	//		Class for representing DXL filter operators
	//
	//---------------------------------------------------------------------------
	class CDXLScalarFilter : public CDXLScalar
	{
		private:
			// private copy ctor
			CDXLScalarFilter(CDXLScalarFilter&);
			
		public:
			// ctor/dtor
			explicit
			CDXLScalarFilter(IMemoryPool *memory_pool);
			
			// accessors
			Edxlopid Edxlop() const;
			const CWStringConst *PstrOpName() const;
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarFilter *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarFilter == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarFilter*>(dxl_op);
			}

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

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure
			virtual
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG	
	};
}
#endif // !GPDXL_CDXLScalarFilter_H

// EOF

