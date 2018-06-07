//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal Inc.
//
//	@filename:
//		CDXLPhysicalCTAS.h
//
//	@doc:
//		Class for representing DXL physical CTAS operators
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLPhysicalCTAS_H
#define GPDXL_CDXLPhysicalCTAS_H

#include "gpos/base.h"
#include "naucrates/md/IMDRelation.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{

	// fwd decl
	class CDXLCtasStorageOptions;
	
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalCTAS
	//
	//	@doc:
	//		Class for representing DXL physical CTAS operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalCTAS : public CDXLPhysical
	{
		private:

			// schema name
			CMDName *m_mdname_schema;
		
			// table name
			CMDName *m_pmdnameRel;
			
			// list of columns
			ColumnDescrDXLArray *m_col_descr_array;
			
			// storage options
			CDXLCtasStorageOptions *m_dxl_ctas_storage_option;
			
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

			// list of vartypmod
			IntPtrArray *m_vartypemod_array;

			// private copy ctor
			CDXLPhysicalCTAS(CDXLPhysicalCTAS&);

		public:
			// ctor
			CDXLPhysicalCTAS
				(
				IMemoryPool *memory_pool, 
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
			~CDXLPhysicalCTAS();

			// operator type
			Edxlopid GetDXLOperator() const;

			// operator name
			const CWStringConst *GetOpNameStr() const;

			// column descriptors
			ColumnDescrDXLArray *GetColumnDescrDXLArray() const
			{
				return m_col_descr_array;
			}
			
			// distribution type
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

			// table name
			CMDName *PmdnameSchema() const
			{
				return m_mdname_schema;
			}
			
			// table name
			CMDName *MdName() const
			{
				return m_pmdnameRel;
			}
			
			// is temporary
			BOOL FTemporary() const
			{
				return m_is_temp_table;
			}

			// CTAS storage options
			CDXLCtasStorageOptions *GetDxlCtasStorageOption() const
			{
				return m_dxl_ctas_storage_option;
			}
			
			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *dxlnode) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *dxlnode, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLPhysicalCTAS *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalCTAS == dxl_op->GetDXLOperator());
				return dynamic_cast<CDXLPhysicalCTAS*>(dxl_op);
			}
	};
}
#endif // !GPDXL_CDXLPhysicalCTAS_H

// EOF
