//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CMDRelationGPDB.h
//
//	@doc:
//		Class representing MD relations
//---------------------------------------------------------------------------



#ifndef GPMD_CMDRelationGPDB_H
#define GPMD_CMDRelationGPDB_H

#include "gpos/base.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDRelation.h"
#include "naucrates/md/IMDColumn.h"

#include "naucrates/md/CMDColumn.h"
#include "naucrates/md/CMDName.h"

namespace gpdxl
{
	class CXMLSerializer;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;


	//---------------------------------------------------------------------------
	//	@class:
	//		CMDRelationGPDB
	//
	//	@doc:
	//		Class representing MD relations
	//
	//---------------------------------------------------------------------------
	class CMDRelationGPDB : public IMDRelation
	{		
		private:
			// memory pool
			IMemoryPool *m_memory_pool;

			// DXL for object
			const CWStringDynamic *m_pstr;
			
			// relation mdid
			IMDId *m_mdid;
			
			// table name
			CMDName *m_mdname;
			
			// is this a temporary relation
			BOOL m_is_temp_table;
			
			// storage type
			Erelstoragetype m_rel_storage_type;
			
			// distribution policy
			GetRelDistrPolicy m_rel_distr_policy;
			
			// columns
			DrgPmdcol *m_md_col_array;
			
			// number of dropped columns
			ULONG m_ulDroppedCols;
			
			// indices of distribution columns
			ULongPtrArray *m_pdrgpulDistrColumns;
			
			// do we need to consider a hash distributed table as random distributed
			BOOL m_fConvertHashToRandom;

			// indices of partition columns
			ULongPtrArray *m_pdrgpulPartColumns;
			
			// partition types
			CharPtrArray *m_pdrgpszPartTypes;

			// number of partition
			ULONG m_ulPartitions;

			// array of key sets
			ULongPtrArray2D *m_pdrgpdrgpulKeys;

			// array of index info
			DrgPmdIndexInfo *m_mdindex_info_array;

			// array of trigger ids
			DrgPmdid *m_pdrgpmdidTriggers;

			// array of check constraint mdids
			DrgPmdid *m_pdrgpmdidCheckConstraint;

			// partition constraint
			IMDPartConstraint *m_pmdpartcnstr;

			// does this table have oids
			BOOL m_has_oids;

			// number of system columns
			ULONG m_ulSystemColumns;
			
			// mapping of column position to positions excluding dropped columns
			HMUlUl *m_phmululNonDroppedCols;
		
			// mapping of attribute number in the system catalog to the positions of
			// the non dropped column in the metadata object
			HMIUl *m_phmiulAttno2Pos;

			// the original positions of all the non-dropped columns
			ULongPtrArray *m_pdrgpulNonDroppedCols;

			// array of column widths including dropped columns
			DrgPdouble *m_pdrgpdoubleColWidths;

			// private copy ctor
			CMDRelationGPDB(const CMDRelationGPDB &);
		
		public:
			
			// ctor
			CMDRelationGPDB
				(
				IMemoryPool *memory_pool,
				IMDId *pmdid,
				CMDName *mdname,
				BOOL fTemporary,
				Erelstoragetype rel_storage_type, 
				GetRelDistrPolicy rel_distr_policy,
				DrgPmdcol *pdrgpmdcol,
				ULongPtrArray *pdrgpulDistrColumns,
				ULongPtrArray *pdrgpulPartColumns,
				CharPtrArray *pdrgpszPartTypes,
				ULONG ulPartitions,
				BOOL fConvertHashToRandom,
				ULongPtrArray2D *ulong_ptr_array_2D,
				DrgPmdIndexInfo *pdrgpmdIndexInfo,
				DrgPmdid *pdrgpmdidTriggers,
				DrgPmdid *pdrgpmdidCheckConstraint,
				IMDPartConstraint *pmdpartcnstr,
				BOOL fHasOids
				);
			
			// dtor
			virtual
			~CMDRelationGPDB();
			
			// accessors
			virtual 
			const CWStringDynamic *Pstr() const
			{
				return m_pstr;
			}	
			
			// the metadata id
			virtual 
			IMDId *MDId() const;
			
			// relation name
			virtual 
			CMDName Mdname() const;
			
			// is this a temp relation
			virtual 
			BOOL IsTemporary() const;
			
			// storage type (heap, appendonly, ...)
			virtual 
			Erelstoragetype GetRelStorageType() const; 
			
			// distribution policy (none, hash, random)
			virtual 
			GetRelDistrPolicy Ereldistribution() const; 
			
			// number of columns
			virtual 
			ULONG UlColumns() const;

			// width of a column with regards to the position
			virtual
			DOUBLE DColWidth(ULONG ulPos) const;

			// does relation have dropped columns
			virtual
			BOOL FHasDroppedColumns() const; 
			
			// number of non-dropped columns
			virtual 
			ULONG UlNonDroppedCols() const; 
			
			// return the absolute position of the given attribute position excluding dropped columns
			virtual 
			ULONG UlPosNonDropped(ULONG ulPos) const;
			
			// return the position of a column in the metadata object given the attribute number in the system catalog
			virtual
			ULONG UlPosFromAttno(INT iAttno) const;

			// return the original positions of all the non-dropped columns
			virtual
			ULongPtrArray *PdrgpulNonDroppedCols() const;

			// number of system columns
			virtual
			ULONG UlSystemColumns() const;

			// retrieve the column at the given position
			virtual 
			const IMDColumn *GetMdCol(ULONG ulPos) const;
			
			// number of key sets
			virtual
			ULONG UlKeySets() const;
			
			// key set at given position
			virtual
			const ULongPtrArray *PdrgpulKeyset(ULONG ulPos) const;
			
			// number of distribution columns
			virtual 
			ULONG UlDistrColumns() const;
			
			// retrieve the column at the given position in the distribution columns list for the relation
			virtual 
			const IMDColumn *PmdcolDistrColumn(ULONG ulPos) const;
			
			// return true if a hash distributed table needs to be considered as random
			virtual 
			BOOL FConvertHashToRandom() const;
			
			// does this table have oids
			virtual
			BOOL HasOids() const;

			// is this a partitioned table
			virtual
			BOOL FPartitioned() const;
			
			// number of partition keys
			virtual
			ULONG UlPartColumns() const;
			
			// number of partitions
			virtual
			ULONG UlPartitions() const;

			// retrieve the partition key column at the given position
			virtual 
			const IMDColumn *PmdcolPartColumn(ULONG ulPos) const;

			// retrieve list of partition types
			virtual
			CharPtrArray *PdrgpszPartTypes() const;

			// retrieve the partition type of the given level
			virtual
			CHAR SzPartType(ULONG ulLevel) const;

			// number of indices
			virtual 
			ULONG UlIndices() const;
			
			// number of triggers
			virtual
			ULONG UlTriggers() const;

			// retrieve the id of the metadata cache index at the given position
			virtual 
			IMDId *PmdidIndex(ULONG ulPos) const;

			// check if index is partial given its mdid
			virtual
			BOOL FPartialIndex(IMDId *pmdid) const;

			// retrieve the id of the metadata cache trigger at the given position
			virtual
			IMDId *PmdidTrigger(ULONG ulPos) const;

			// serialize metadata relation in DXL format given a serializer object
			virtual 
			void Serialize(gpdxl::CXMLSerializer *) const;

			// number of check constraints
			virtual
			ULONG UlCheckConstraints() const;

			// retrieve the id of the check constraint cache at the given position
			virtual
			IMDId *PmdidCheckConstraint(ULONG ulPos) const;

			// part constraint
			virtual
			IMDPartConstraint *Pmdpartcnstr() const;

#ifdef GPOS_DEBUG
			// debug print of the metadata relation
			virtual 
			void DebugPrint(IOstream &os) const;
#endif
	};
}



#endif // !GPMD_CMDRelationGPDB_H

// EOF
