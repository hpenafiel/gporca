//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLPhysicalDynamicBitmapTableScan.h
//
//	@doc:
//		Class for representing dynamic DXL bitmap table scan operators.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalDynamicBitmapTableScan_H
#define GPDXL_CDXLPhysicalDynamicBitmapTableScan_H

#include "gpos/base.h"

#include "naucrates/dxl/operators/CDXLPhysicalAbstractBitmapScan.h"

namespace gpdxl
{
	using namespace gpos;

	// fwd declarations
	class CDXLTableDescr;
	class CXMLSerializer;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalDynamicBitmapTableScan
	//
	//	@doc:
	//		Class for representing DXL bitmap table scan operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalDynamicBitmapTableScan : public CDXLPhysicalAbstractBitmapScan
	{
		private:
			// id of partition index structure
			ULONG m_ulPartIndexId;

			// printable partition index id
			ULONG m_ulPartIndexIdPrintable;

			// private copy ctor
			CDXLPhysicalDynamicBitmapTableScan(const CDXLPhysicalDynamicBitmapTableScan &);

		public:
			// ctor
			CDXLPhysicalDynamicBitmapTableScan
				(
				IMemoryPool *memory_pool,
				CDXLTableDescr *table_descr,
				ULONG part_idx_id,
				ULONG part_idx_id_printable
				)
				:
				CDXLPhysicalAbstractBitmapScan(memory_pool, table_descr),
				m_ulPartIndexId(part_idx_id),
				m_ulPartIndexIdPrintable(part_idx_id_printable)
			{
				GPOS_ASSERT(NULL != table_descr);
			}

			// dtor
			virtual
			~CDXLPhysicalDynamicBitmapTableScan()
			{}

			// operator type
			virtual
			Edxlopid GetDXLOperator() const
			{
				return EdxlopPhysicalDynamicBitmapTableScan;
			}

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// partition index id
			ULONG UlPartIndexId() const
			{
				return m_ulPartIndexId;
			}

			// printable partition index id
			ULONG UlPartIndexIdPrintable() const
			{
				return m_ulPartIndexIdPrintable;
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalDynamicBitmapTableScan *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalDynamicBitmapTableScan == dxl_op->GetDXLOperator());

 	 	 		return dynamic_cast<CDXLPhysicalDynamicBitmapTableScan *>(dxl_op);
			}

	};  // class CDXLPhysicalDynamicBitmapTableScan
}

#endif  // !GPDXL_CDXLPhysicalDynamicBitmapTableScan_H

// EOF
