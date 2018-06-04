//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalInsert.h
//
//	@doc:
//		Class for representing logical insert operators
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalInsert_H
#define GPDXL_CDXLLogicalInsert_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogical.h"

namespace gpdxl
{
	using namespace gpmd;

	// fwd decl
	class CDXLTableDescr;
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalInsert
	//
	//	@doc:
	//		Class for representing logical insert operators
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalInsert : public CDXLLogical
	{
		private:

			// target table descriptor
			CDXLTableDescr *m_table_descr_dxl;

			// list of source column ids		
			ULongPtrArray *m_pdrgpul;
			
			// private copy ctor
			CDXLLogicalInsert(const CDXLLogicalInsert &);
			
		public:
			
			// ctor/dtor
			CDXLLogicalInsert(IMemoryPool *memory_pool, CDXLTableDescr *pdxltabdesc, ULongPtrArray *pdrgpul);
						
			virtual
			~CDXLLogicalInsert();
		
			// operator type
			Edxlopid GetDXLOperator() const;

			// operator name
			const CWStringConst *GetOpNameStr() const;

			// target table descriptor 
			CDXLTableDescr *GetTableDescr() const
			{
				return m_table_descr_dxl;
			}
			
			// source column ids
			ULongPtrArray *Pdrgpul() const
			{
				return m_pdrgpul;
			}
			
#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLLogicalInsert *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalInsert == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLLogicalInsert*>(dxl_op);
			}

	};
}

#endif // !GPDXL_CDXLLogicalInsert_H

// EOF

