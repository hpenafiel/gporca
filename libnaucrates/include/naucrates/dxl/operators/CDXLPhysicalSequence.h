//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalSequence.h
//
//	@doc:
//		Class for representing DXL physical sequence operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalSequence_H
#define GPDXL_CDXLPhysicalSequence_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLSpoolInfo.h"


namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalSequence
	//
	//	@doc:
	//		Class for representing DXL physical sequence operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalSequence : public CDXLPhysical
	{
		private:

			// private copy ctor
			CDXLPhysicalSequence(CDXLPhysicalSequence&);

		public:
			// ctor
			CDXLPhysicalSequence(IMemoryPool *memory_pool);
			
			// dtor
			virtual
			~CDXLPhysicalSequence();
			
			// accessors
			Edxlopid Edxlop() const;
			const CWStringConst *PstrOpName() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalSequence *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalSequence == dxl_op->Edxlop());

				return dynamic_cast<CDXLPhysicalSequence*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLPhysicalSequence_H

// EOF

