//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLScalarBitmapIndexProbe.h
//
//	@doc:
//		Class for representing DXL bitmap index probe operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarBitmapIndexProbe_H
#define GPDXL_CDXLScalarBitmapIndexProbe_H

#include "gpos/base.h"

#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpdxl
{
	using namespace gpos;

	// fwd declarations
	class CDXLIndexDescr;
	class CXMLSerializer;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarBitmapIndexProbe
	//
	//	@doc:
	//		Class for representing DXL bitmap index probe operators
	//
	//---------------------------------------------------------------------------
	class CDXLScalarBitmapIndexProbe : public CDXLScalar
	{
		private:
			// index descriptor associated with the scanned table
			CDXLIndexDescr *m_index_descr_dxl;

			// disable copy ctor
			CDXLScalarBitmapIndexProbe(CDXLScalarBitmapIndexProbe &);

		public:
			// ctor
			CDXLScalarBitmapIndexProbe
				(
				IMemoryPool *memory_pool,
				CDXLIndexDescr *index_descr_dxl
				);

			//dtor
			virtual
			~CDXLScalarBitmapIndexProbe();

			// operator type
			virtual
			Edxlopid GetDXLOperator() const
			{
				return EdxlopScalarBitmapIndexProbe;
			}

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// index descriptor
			virtual
			const CDXLIndexDescr *GetIndexDescr() const
			{
				return m_index_descr_dxl;
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

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
			virtual
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLScalarBitmapIndexProbe *Cast
				(
					CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarBitmapIndexProbe== dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarBitmapIndexProbe *>(dxl_op);
			}

	};  // class CDXLScalarBitmapIndexProbe
}

#endif  // !GPDXL_CDXLScalarBitmapIndexProbe_H

// EOF
