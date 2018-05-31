//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLWindowSpec.h
//
//	@doc:
//		Class for representing DXL window specification in the DXL
//		representation of the logical query tree
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLWindowSpec_H
#define GPDXL_CDXLWindowSpec_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLWindowFrame.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/md/CMDName.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLWindowSpec
	//
	//	@doc:
	//		Class for representing DXL window specification in the DXL
	//		representation of the logical query tree
	//
	//---------------------------------------------------------------------------
	class CDXLWindowSpec : public CRefCount
	{
		private:

			// memory pool;
			IMemoryPool *m_memory_pool;

			// partition-by column identifiers
			ULongPtrArray *m_pdrgpulPartCol;

			// name of window specification
			CMDName *m_mdname;

			// sorting columns
			CDXLNode *m_sort_col_list_dxl;

			// window frame associated with the window key
			CDXLWindowFrame *m_pdxlwf;

			// private copy ctor
			CDXLWindowSpec(const CDXLWindowSpec&);

		public:

			// ctor
			CDXLWindowSpec
				(
				IMemoryPool *memory_pool,
				ULongPtrArray *pdrgpulPartCol,
				CMDName *pmdname,
				CDXLNode *sort_col_list_dxl,
				CDXLWindowFrame *pdxlwf
				);

			// dtor
			virtual
			~CDXLWindowSpec();

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *) const;

			// set window frame definition
			void SetWindowFrame(CDXLWindowFrame *pdxlwf);

			// return window frame
			CDXLWindowFrame *GetWindowFrame() const
			{
				return m_pdxlwf;
			}

			// partition-by column identifiers
			ULongPtrArray *PdrgulPartColList() const
			{
				return m_pdrgpulPartCol;
			}

			// sort columns
			CDXLNode *GetSortColListDXL() const
			{
				return m_sort_col_list_dxl;
			}

			// window specification name
			CMDName *MdName() const
			{
				return m_mdname;
			}
	};

	typedef CDynamicPtrArray<CDXLWindowSpec, CleanupRelease> DrgPdxlws;
}
#endif // !GPDXL_CDXLWindowSpec_H

// EOF
