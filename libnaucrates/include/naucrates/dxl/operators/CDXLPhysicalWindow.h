//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalWindow.h
//
//	@doc:
//		Class for representing DXL window operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalWindow_H
#define GPDXL_CDXLPhysicalWindow_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLWindowKey.h"

namespace gpdxl
{
	// indices of window elements in the children array
	enum Edxlwindow
	{
		EdxlwindowIndexProjList = 0,
		EdxlwindowIndexFilter,
		EdxlwindowIndexChild,
		EdxlwindowIndexSentinel
	};

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalWindow
	//
	//	@doc:
	//		Class for representing DXL window operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalWindow : public CDXLPhysical
	{
		private:

			// partition columns
			ULongPtrArray *m_pdrgpulPartCols;

			// window keys
			CDXLWindowKeyArray *m_pdrgpdxlwk;

			// private copy ctor
			CDXLPhysicalWindow(CDXLPhysicalWindow&);

		public:

			//ctor
			CDXLPhysicalWindow(IMemoryPool *memory_pool, ULongPtrArray *pdrgpulPartCols, CDXLWindowKeyArray *pdrgpdxlwk);

			//dtor
			virtual
			~CDXLPhysicalWindow();

			// accessors
			Edxlopid GetDXLOperator() const;
			const CWStringConst *GetOpNameStr() const;

			// number of partition columns
			ULONG UlPartCols() const;

			// return partition columns
			const ULongPtrArray *PrgpulPartCols() const
			{
				return m_pdrgpulPartCols;
			}

			// number of window keys
			ULONG UlWindowKeys() const;

			// return the window key at a given position
			CDXLWindowKey *PdxlWindowKey(ULONG ulPos) const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *dxlnode) const;

			// conversion function
			static
			CDXLPhysicalWindow *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalWindow == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLPhysicalWindow*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLPhysicalWindow_H

// EOF

