//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalIndexOnlyScan.h
//
//	@doc:
//		Class for representing DXL index only scan operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalIndexOnlyScan_H
#define GPDXL_CDXLPhysicalIndexOnlyScan_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLPhysicalIndexScan.h"
#include "naucrates/dxl/operators/CDXLIndexDescr.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalIndexOnlyScan
	//
	//	@doc:
	//		Class for representing DXL index only scan operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalIndexOnlyScan : public CDXLPhysicalIndexScan
	{
		private:

			// private copy ctor
			CDXLPhysicalIndexOnlyScan(CDXLPhysicalIndexOnlyScan&);

		public:

			//ctor
			CDXLPhysicalIndexOnlyScan
				(
				IMemoryPool *memory_pool,
				CDXLTableDescr *pdxltabdesc,
				CDXLIndexDescr *pdxlid,
				EdxlIndexScanDirection edxlisd
				);

			//dtor
			virtual
			~CDXLPhysicalIndexOnlyScan()
			{}

			// operator type
			virtual
			Edxlopid Edxlop() const;

			// operator name
			virtual
			const CWStringConst *PstrOpName() const;

			// conversion function
			static
			CDXLPhysicalIndexOnlyScan *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalIndexOnlyScan == dxl_op->Edxlop());

				return dynamic_cast<CDXLPhysicalIndexOnlyScan*>(dxl_op);
			}
	};
}
#endif // !GPDXL_CDXLPhysicalIndexOnlyScan_H

// EOF

