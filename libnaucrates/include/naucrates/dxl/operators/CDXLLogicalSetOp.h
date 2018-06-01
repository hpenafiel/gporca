//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalSetOp.h
//
//	@doc:
//		Class for representing DXL logical set operators
//		
//	@owner: 
//		
//
//	@test:
//
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLLogicalSetOp_H
#define GPDXL_CDXLLogicalSetOp_H

#include "gpos/base.h"

#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"

namespace gpdxl
{

	enum EdxlSetOpType
	{
		EdxlsetopUnion,
		EdxlsetopUnionAll,
		EdxlsetopIntersect,
		EdxlsetopIntersectAll,
		EdxlsetopDifference,
		EdxlsetopDifferenceAll,
		EdxlsetopSentinel
	};

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalSetOp
	//
	//	@doc:
	//		Class for representing DXL logical set operators
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalSetOp : public CDXLLogical
	{
		private:

			// private copy ctor
			CDXLLogicalSetOp(CDXLLogicalSetOp&);

			// set operation type
			EdxlSetOpType m_edxlsetoptype;

			// list of output column descriptors
			ColumnDescrDXLArray *m_pdrgpdxlcd;

			// array of input colid arrays
			ULongPtrArray2D *m_pdrgpdrgpul;
			
			// do the columns need to be casted accross inputs
			BOOL m_fCastAcrossInputs;

		public:
			// ctor
			CDXLLogicalSetOp
				(
				IMemoryPool *memory_pool,
				EdxlSetOpType edxlsetoptype,
				ColumnDescrDXLArray *pdrgdxlcd,
				ULongPtrArray2D *ulong_ptr_array_2D,
				BOOL fCastAcrossInput
				);

			// dtor
			virtual
			~CDXLLogicalSetOp();

			// operator id
			Edxlopid GetDXLOperator() const;

			// operator name
			const CWStringConst *GetOpNameStr() const;

			// set operator type
			EdxlSetOpType Edxlsetoptype() const
			{
				return m_edxlsetoptype;
			}

			// array of output columns
			const ColumnDescrDXLArray *GetColumnDescrDXLArray() const
			{
				return m_pdrgpdxlcd;
			}

			// number of output columns
			ULONG Arity() const
			{
				return m_pdrgpdxlcd->Size();
			}

			// output column descriptor at a given position
			const CDXLColDescr *GetColumnDescrAt
				(
				ULONG ulPos
				)
				const
			{
				return (*m_pdrgpdxlcd)[ulPos];
			}

			// number of inputs to the n-ary set operation
		    ULONG UlChildren() const
			{
				return m_pdrgpdrgpul->Size();	
			}
		
			// column array of the input at a given position 
			const ULongPtrArray *Pdrgpul
				(
				ULONG ulPos
				)
				const
			{
				GPOS_ASSERT(ulPos < UlChildren());
				
				return (*m_pdrgpdrgpul)[ulPos];
			}
		
			// do the columns across inputs need to be casted
			BOOL FCastAcrossInputs() const
			{
				return m_fCastAcrossInputs;
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// check if given column is defined by operator
			virtual
			BOOL FDefinesColumn(ULONG ulColId) const;

			// conversion function
			static
			CDXLLogicalSetOp *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalSetOp == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLLogicalSetOp*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLLogicalSetOp_H

// EOF
