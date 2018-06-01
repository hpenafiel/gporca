//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarHashExprList.h
//
//	@doc:
//		Class for representing hash expressions list in DXL Redistribute motion nodes.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarHashExprList_H
#define GPDXL_CDXLScalarHashExprList_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"


namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarHashExprList
	//
	//	@doc:
	//		Hash expressions list in Redistribute motion nodes
	//
	//---------------------------------------------------------------------------
	class CDXLScalarHashExprList : public CDXLScalar
	{
		private:
		
			// private copy ctor
			CDXLScalarHashExprList(CDXLScalarHashExprList&);
			
		public:
			// ctor/dtor
			explicit
			CDXLScalarHashExprList(IMemoryPool *memory_pool);
			
			virtual
			~CDXLScalarHashExprList(){};

			// ident accessors
			Edxlopid Edxlop() const;
			
			// name of the operator
			const CWStringConst *PstrOpName() const;
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarHashExprList *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarHashExprList == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarHashExprList*>(dxl_op);
			}

			// does the operator return a boolean result
			virtual
			BOOL FBoolean
					(
					CMDAccessor *//pmda
					)
					const
			{
				GPOS_ASSERT(!"Invalid function call on a container operator");
				return false;
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
	};
}

#endif // !GPDXL_CDXLScalarHashExprList_H

// EOF
