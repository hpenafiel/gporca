//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarSortColList.h
//
//	@doc:
//		Class for representing a list of sorting columns in a DXL Sort and Motion nodes.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarSortColList_H
#define GPDXL_CDXLScalarSortColList_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpdxl
{

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarSortColList
	//
	//	@doc:
	//		Sorting column lists in DXL Sort And Motion nodes
	//
	//---------------------------------------------------------------------------
	class CDXLScalarSortColList : public CDXLScalar
	{
		private:
		
			// private copy ctor
			CDXLScalarSortColList(CDXLScalarSortColList&);
			
		public:
			// ctor/dtor
			explicit
			CDXLScalarSortColList(IMemoryPool *memory_pool);
			
			virtual
			~CDXLScalarSortColList(){};

			// ident accessors
			Edxlopid Edxlop() const;
			
			// name of the operator
			const CWStringConst *PstrOpName() const;
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *, const CDXLNode *) const;

			// conversion function
			static
			CDXLScalarSortColList *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarSortColList == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarSortColList*>(dxl_op);
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
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
			
	};
}

#endif // !GPDXL_CDXLScalarSortColList_H

// EOF
