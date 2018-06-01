//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalWindow.h
//
//	@doc:
//		Class for representing DXL logical window operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalWindow_H
#define GPDXL_CDXLLogicalWindow_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLWindowSpec.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalWindow
	//
	//	@doc:
	//		Class for representing DXL logical window operators
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalWindow : public CDXLLogical
	{
		private:
			// array of window specifications
			DXLWindowSpecArray *m_pdrgpdxlws;

			// private copy ctor
			CDXLLogicalWindow(CDXLLogicalWindow&);

		public:

			//ctor
			CDXLLogicalWindow(IMemoryPool *memory_pool, DXLWindowSpecArray *pdrgpdxlwinspec);

			//dtor
			virtual
			~CDXLLogicalWindow();

			// accessors
			Edxlopid Edxlop() const;
			const CWStringConst *PstrOpName() const;

			// number of window specs
			ULONG UlWindowSpecs() const;

			// return the window key at a given position
			CDXLWindowSpec *Pdxlws(ULONG ulPos) const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLLogicalWindow *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalWindow == dxl_op->Edxlop());

				return dynamic_cast<CDXLLogicalWindow*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLLogicalWindow_H

// EOF

