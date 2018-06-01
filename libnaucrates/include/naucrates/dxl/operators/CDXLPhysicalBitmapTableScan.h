//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLPhysicalBitmapTableScan.h
//
//	@doc:
//		Class for representing DXL bitmap table scan operators.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalBitmapTableScan_H
#define GPDXL_CDXLPhysicalBitmapTableScan_H

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
	//		CDXLPhysicalBitmapTableScan
	//
	//	@doc:
	//		Class for representing DXL bitmap table scan operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalBitmapTableScan : public CDXLPhysicalAbstractBitmapScan
	{
		private:
			// private copy ctor
			CDXLPhysicalBitmapTableScan(const CDXLPhysicalBitmapTableScan &);

		public:
			// ctors
			CDXLPhysicalBitmapTableScan
				(
				IMemoryPool *memory_pool,
				CDXLTableDescr *pdxltabdesc
				)
				:
				CDXLPhysicalAbstractBitmapScan(memory_pool, pdxltabdesc)
			{
			}

			// dtor
			virtual
			~CDXLPhysicalBitmapTableScan()
			{}

			// operator type
			virtual
			Edxlopid Edxlop() const
			{
				return EdxlopPhysicalBitmapTableScan;
			}

			// operator name
			virtual
			const CWStringConst *PstrOpName() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalBitmapTableScan *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalBitmapTableScan == dxl_op->Edxlop());

 	 	 		return dynamic_cast<CDXLPhysicalBitmapTableScan *>(dxl_op);
			}

	};  // class CDXLPhysicalBitmapTableScan
}

#endif  // !GPDXL_CDXLPhysicalBitmapTableScan_H

// EOF
