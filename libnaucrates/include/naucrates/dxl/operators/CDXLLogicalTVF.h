//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLLogicalTVF.h
//
//	@doc:
//		Class for representing table-valued functions
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalTVF_H
#define GPDXL_CDXLLogicalTVF_H

#include "gpos/base.h"
#include "naucrates/md/IMDId.h"
#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"

namespace gpdxl
{
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalTVF
	//
	//	@doc:
	//		Class for representing table-valued functions
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalTVF : public CDXLLogical
	{
		private:
			// catalog id of the function
			IMDId *m_pmdidFunc;

			// return type
			IMDId *m_pmdidRetType;

			// function name
			CMDName *m_mdname;

			// list of column descriptors		
			ColumnDescrDXLArray *m_pdrgdxlcd;
			
			// private copy ctor
			CDXLLogicalTVF(const CDXLLogicalTVF &);
			
		public:
			// ctor/dtor
			CDXLLogicalTVF
				(
				IMemoryPool *memory_pool,
				IMDId *pmdidFunc,
				IMDId *pmdidRetType,
				CMDName *mdname,
				ColumnDescrDXLArray *pdrgdxlcd
				);
						
			virtual
			~CDXLLogicalTVF();
		
			// get operator type
			Edxlopid GetDXLOperator() const;

			// get operator name
			const CWStringConst *GetOpNameStr() const;

			// get function name
			CMDName *MdName() const
			{
				return m_mdname;
			}

			// get function id
			IMDId *PmdidFunc() const
			{
				return m_pmdidFunc;
			}

			// get return type
			IMDId *PmdidRetType() const
			{
				return m_pmdidRetType;
			}

			// get number of output columns
			ULONG Arity() const;
			
			// return the array of column descriptors
			const ColumnDescrDXLArray *GetColumnDescrDXLArray() const
			{
				return m_pdrgdxlcd;
			}

			// get the column descriptor at the given position
			const CDXLColDescr *GetColumnDescrAt(ULONG ul) const;

			// check if given column is defined by operator
			virtual
			BOOL IsColDefined(ULONG col_id) const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLLogicalTVF *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalTVF == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLLogicalTVF*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}

#endif // !GPDXL_CDXLLogicalTVF_H

// EOF

