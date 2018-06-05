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
			CMDName *m_mdname_schema;
			
			// table name
			CMDName *m_pmdnameRel;
			
			// list of columns
			ColumnDescrDXLArray *m_col_descr_array;
			
			// storage options
			CDXLCtasStorageOptions *m_pdxlctasopt;
			
			// distribution policy
			IMDRelation::Ereldistrpolicy m_rel_distr_policy;

			// list of distribution column positions		
			ULongPtrArray *m_distr_column_pos_array;
			
			// is this a temporary table
			BOOL m_is_temp_table;
			
			// does table have oids
			BOOL m_has_oids;
			
			// storage type
			IMDRelation::Erelstoragetype m_rel_storage_type;
			
			// list of source column ids		
			ULongPtrArray *m_src_colids_array;
			
			// list of vartypmod for target expressions
			// typemod records type-specific, e.g. the maximum length of a character column
			IntPtrArray *m_vartypemod_array;

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
				IMDRelation::Ereldistrpolicy rel_distr_policy,
				ULongPtrArray *pdrgpulDistr, 
				BOOL fTemporary, 
				BOOL fHasOids, 
				IMDRelation::Erelstoragetype rel_storage_type,
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
				return m_mdname_schema;
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
				return m_rel_storage_type;
			}
			
			// distribution policy
			IMDRelation::Ereldistrpolicy Ereldistrpolicy() const
			{
				return m_rel_distr_policy;
			}
			
			// distribution column positions
			ULongPtrArray *PdrgpulDistr() const
			{
				return m_distr_column_pos_array;
			}
		
			// source column ids
			ULongPtrArray *PdrgpulSource() const
			{
				return m_src_colids_array;
			}
			
			// list of vartypmod for target expressions
			IntPtrArray *PdrgpiVarTypeMod() const
			{
				return m_vartypemod_array;
			}

			// is it a temporary table
			BOOL FTemporary() const
			{
				return m_is_temp_table;
			}
			
			// does the table have oids
			BOOL FHasOids() const
			{
				return m_has_oids;
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

