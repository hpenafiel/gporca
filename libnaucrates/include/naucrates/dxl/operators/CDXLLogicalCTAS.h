//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLLogicalCTAS.h
//
//	@doc:
//		Class for representing logical "CREATE TABLE AS" (CTAS) operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalCTAS_H
#define GPDXL_CDXLLogicalCTAS_H

#include "gpos/base.h"
#include "naucrates/md/IMDRelation.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"

#include "naucrates/dxl/operators/CDXLLogical.h"

namespace gpdxl
{

	// fwd decl
	class CDXLCtasStorageOptions;
	
	using namespace gpmd;

	
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalCTAS
	//
	//	@doc:
	//		Class for representing logical "CREATE TABLE AS" (CTAS) operator
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalCTAS : public CDXLLogical
	{
		private:
			
			// mdid of table to create
			IMDId *m_mdid;
			
			// schema name
			CMDName *m_pmdnameSchema;
			
			// table name
			CMDName *m_pmdnameRel;
			
			// list of columns
			ColumnDescrDXLArray *m_col_descr_array;
			
			// storage options
			CDXLCtasStorageOptions *m_pdxlctasopt;
			
			// distribution policy
			IMDRelation::Ereldistrpolicy m_ereldistrpolicy;

			// list of distribution column positions		
			ULongPtrArray *m_pdrgpulDistr;
			
			// is this a temporary table
			BOOL m_fTemporary;
			
			// does table have oids
			BOOL m_fHasOids;
			
			// storage type
			IMDRelation::Erelstoragetype m_erelstorage;
			
			// list of source column ids		
			ULongPtrArray *m_pdrgpulSource;
			
			// list of vartypmod for target expressions
			// typemod records type-specific, e.g. the maximum length of a character column
			IntPtrArray *m_pdrgpiVarTypeMod;

			// private copy ctor
			CDXLLogicalCTAS(const CDXLLogicalCTAS &);
			
		public:
			
			// ctor
			CDXLLogicalCTAS
				(
				IMemoryPool *memory_pool, 
				IMDId *pmdid,
				CMDName *pmdnameSchema, 
				CMDName *pmdnameRel, 
				ColumnDescrDXLArray *pdrgpdxcd,
				CDXLCtasStorageOptions *pdxlctasopt,
				IMDRelation::Ereldistrpolicy ereldistrpolicy,
				ULongPtrArray *pdrgpulDistr, 
				BOOL fTemporary, 
				BOOL fHasOids, 
				IMDRelation::Erelstoragetype erelstorage,
				ULongPtrArray *pdrgpulSource,
				IntPtrArray *pdrgpiVarTypeMod
				);
				
			// dtor
			virtual
			~CDXLLogicalCTAS();
		
			// operator type
			Edxlopid GetDXLOperator() const;

			// operator name
			const CWStringConst *GetOpNameStr() const;

			// mdid of table to create
			IMDId *MDId() const
			{
				return m_mdid;
			}
			
			// schema name
			CMDName *PmdnameSchema() const
			{
				return m_pmdnameSchema;
			}
			
			// table name
			CMDName *MdName() const
			{
				return m_pmdnameRel;
			}
			
			// column descriptors
			ColumnDescrDXLArray *GetColumnDescrDXLArray() const
			{
				return m_col_descr_array;
			}
			
			// storage type
			IMDRelation::Erelstoragetype Erelstorage() const
			{
				return m_erelstorage;
			}
			
			// distribution policy
			IMDRelation::Ereldistrpolicy Ereldistrpolicy() const
			{
				return m_ereldistrpolicy;
			}
			
			// distribution column positions
			ULongPtrArray *PdrgpulDistr() const
			{
				return m_pdrgpulDistr;
			}
		
			// source column ids
			ULongPtrArray *PdrgpulSource() const
			{
				return m_pdrgpulSource;
			}
			
			// list of vartypmod for target expressions
			IntPtrArray *PdrgpiVarTypeMod() const
			{
				return m_pdrgpiVarTypeMod;
			}

			// is it a temporary table
			BOOL FTemporary() const
			{
				return m_fTemporary;
			}
			
			// does the table have oids
			BOOL FHasOids() const
			{
				return m_fHasOids;
			}
			
			// CTAS storage options
			CDXLCtasStorageOptions *Pdxlctasopt() const
			{
				return m_pdxlctasopt;
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// check if given column is defined by operator
			virtual
			BOOL IsColDefined(ULONG col_id) const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLLogicalCTAS *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalCTAS == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLLogicalCTAS*>(dxl_op);
			}

	};
}

#endif // !GPDXL_CDXLLogicalCTAS_H

// EOF

