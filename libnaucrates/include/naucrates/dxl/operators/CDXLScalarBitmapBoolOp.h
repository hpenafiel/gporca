//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLScalarBitmapBoolOp.h
//
//	@doc:
//		Class for representing DXL bitmap boolean operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarBitmapBoolOp_H
#define GPDXL_CDXLScalarBitmapBoolOp_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"


namespace gpdxl
{
	using namespace gpos;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarBitmapBoolOp
	//
	//	@doc:
	//		Class for representing DXL bitmap boolean operator
	//
	//---------------------------------------------------------------------------
	class CDXLScalarBitmapBoolOp : public CDXLScalar
	{

		public:
		
			// type of bitmap operator
			enum EdxlBitmapBoolOp
			{
				EdxlbitmapAnd,
				EdxlbitmapOr,
				EdxlbitmapSentinel
			};
		
		private:

			// type id
			IMDId *m_mdid_type;
			
			// operator type
			const EdxlBitmapBoolOp m_bitmapboolop;

			// private copy ctor
			CDXLScalarBitmapBoolOp(const CDXLScalarBitmapBoolOp&);

		public:
			// ctor
			CDXLScalarBitmapBoolOp(IMemoryPool *memory_pool, IMDId *mdid_type, EdxlBitmapBoolOp bitmapboolop);
			
			// dtor 
			virtual
			~CDXLScalarBitmapBoolOp();

			// dxl operator type
			virtual
			Edxlopid Edxlop() const;

			// bitmap operator type
			EdxlBitmapBoolOp Edxlbitmapboolop() const;
			
			// return type
			IMDId *MDIdType() const;

			// name of the DXL operator name
			virtual
			const CWStringConst *PstrOpName() const;

			// does the operator return a boolean result
			virtual
			BOOL FBoolean(CMDAccessor *pmda) const;
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;


#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			virtual
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLScalarBitmapBoolOp *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarBitmapBoolOp == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarBitmapBoolOp*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLScalarBitmapBoolOp_H

// EOF
