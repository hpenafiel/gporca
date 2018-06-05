//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDynamicIndexScan.h
//
//	@doc:
//		Class for representing DXL dynamic index scan operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalDynamicIndexScan_H
#define GPDXL_CDXLPhysicalDynamicIndexScan_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{


	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalDynamicIndexScan
	//
	//	@doc:
	//		Class for representing DXL dynamic index scan operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalDynamicIndexScan : public CDXLPhysical
	{
		private:

			// table descriptor for the scanned table
			CDXLTableDescr *m_table_descr_dxl;

			// part index id
			ULONG m_ulPartIndexId;
			
			// printable partition index id
			ULONG m_ulPartIndexIdPrintable;

			// index descriptor associated with the scanned table
			CDXLIndexDescr *m_index_descr_dxl;

			// scan direction of the index
			EdxlIndexScanDirection m_edxlisd;

			// private copy ctor
			CDXLPhysicalDynamicIndexScan(CDXLPhysicalDynamicIndexScan&);

		public:

			// indices of dynamic index scan elements in the children array
			enum Edxldis
			{
				EdxldisIndexProjList = 0,
				EdxldisIndexFilter,
				EdxldisIndexCondition,
				EdxldisSentinel
			};
			
			//ctor
			CDXLPhysicalDynamicIndexScan
				(
				IMemoryPool *memory_pool,
				CDXLTableDescr *table_descr,
				ULONG part_idx_id,
				ULONG part_idx_id_printable,
				CDXLIndexDescr *pdxlid,
				EdxlIndexScanDirection idx_scan_direction
				);

			//dtor
			virtual
			~CDXLPhysicalDynamicIndexScan();

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// index descriptor
			const CDXLIndexDescr *GetIndexDescr() const;

			//table descriptor
			const CDXLTableDescr *GetTableDescr() const;

			// partition index id
			ULONG UlPartIndexId() const;
			
			// printable partition index id
			ULONG UlPartIndexIdPrintable() const;

			// scan direction
			EdxlIndexScanDirection EdxlScanDirection() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalDynamicIndexScan *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalDynamicIndexScan == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLPhysicalDynamicIndexScan*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLPhysicalDynamicIndexScan_H

// EOF

