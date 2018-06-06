//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarArray.h
//
//	@doc:
//		Class for representing DXL scalar arrays
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarArray_H
#define GPDXL_CDXLScalarArray_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarArray
	//
	//	@doc:
	//		Class for representing DXL arrays
	//
	//---------------------------------------------------------------------------
	class CDXLScalarArray : public CDXLScalar
	{
		private:

			// base element type id
			IMDId *m_pmdidElem;
			
			// array type id
			IMDId *m_pmdidArray;

			// is it a multidimensional array
			BOOL m_fMultiDimensional;

			// private copy ctor
			CDXLScalarArray(const CDXLScalarArray&);

		public:
			// ctor
			CDXLScalarArray
				(
				IMemoryPool *memory_pool,
				IMDId *pmdidElem,
				IMDId *pmdidArray,
				BOOL fMultiDimensional
				);

			// dtor
			virtual
			~CDXLScalarArray();

			// ident accessors
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// element type id
			IMDId *PmdidElem() const;

			// array type id
			IMDId *PmdidArray() const;

			// is array multi-dimensional 
			BOOL FMultiDimensional() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarArray *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarArray == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarArray*>(dxl_op);
			}

			// does the operator return a boolean result
			virtual
			BOOL HasBoolResult
					(
					CMDAccessor *//md_accessor
					)
					const
			{
				return false;
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
	};
}

#endif // !GPDXL_CDXLScalarArray_H

// EOF
