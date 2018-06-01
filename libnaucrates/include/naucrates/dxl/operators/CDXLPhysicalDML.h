//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalDML.h
//
//	@doc:
//		Class for representing physical DML operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalDML_H
#define GPDXL_CDXLPhysicalDML_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{
	// fwd decl
	class CDXLTableDescr;
	class CDXLDirectDispatchInfo;
	
	enum EdxlDmlType
		{
			Edxldmlinsert,
			Edxldmldelete,
			Edxldmlupdate,
			EdxldmlSentinel
		};

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalDML
	//
	//	@doc:
	//		Class for representing physical DML operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalDML : public CDXLPhysical
	{
		private:

			// operator type
			const EdxlDmlType m_edxldmltype;

			// target table descriptor
			CDXLTableDescr *m_pdxltabdesc;

			// list of source column ids		
			ULongPtrArray *m_pdrgpul;
			
			// action column id
			ULONG m_ulAction;

			// oid column id
			ULONG m_ulOid;

			// ctid column id
			ULONG m_ulCtid;

			// segmentid column id
			ULONG m_ulSegmentId;

			// should update preserve tuple oids
			BOOL m_fPreserveOids;	

			// tuple oid column id
			ULONG m_ulTupleOid;
			
			// direct dispatch info for insert statements 
			CDXLDirectDispatchInfo *m_direct_dispatch_info;
			
			// needs the data to be sorted or not
			BOOL m_fInputSorted;

			// private copy ctor
			CDXLPhysicalDML(const CDXLPhysicalDML &);
			
		public:
			
			// ctor
			CDXLPhysicalDML
				(
				IMemoryPool *memory_pool,
				const EdxlDmlType edxldmltype,
				CDXLTableDescr *pdxltabdesc,
				ULongPtrArray *pdrgpul,
				ULONG ulAction,
				ULONG ulOid,
				ULONG ulCtid,
				ULONG ulSegmentId,
				BOOL fPreserveOids,
				ULONG ulTupleOid,
				CDXLDirectDispatchInfo *dxl_direct_dispatch_info,
				BOOL fInputSorted
				);

			// dtor
			virtual
			~CDXLPhysicalDML();
		
			// operator type
			Edxlopid GetDXLOperator() const;

			// operator name
			const CWStringConst *GetOpNameStr() const;

			// DML operator type
			EdxlDmlType EdxlDmlOpType() const
			{
				return m_edxldmltype;
			}

			// target table descriptor 
			CDXLTableDescr *Pdxltabdesc() const
			{
				return m_pdxltabdesc;
			}
			
			// source column ids
			ULongPtrArray *Pdrgpul() const
			{
				return m_pdrgpul;
			}

			// action column id
			ULONG UlAction() const
			{
				return m_ulAction;
			}

			// oid column id
			ULONG UlOid() const
			{
				return m_ulOid;
			}

			// ctid column id
			ULONG UlCtid() const
			{
				return m_ulCtid;
			}

			// segmentid column id
			ULONG UlSegmentId() const
			{
				return m_ulSegmentId;
			}
			
			// does update preserve oids
			BOOL FPreserveOids() const
			{
				return m_fPreserveOids;
			}

			// tuple oid column id
			ULONG UlTupleOid() const
			{
				return m_ulTupleOid;
			}
			
			// direct dispatch info
			CDXLDirectDispatchInfo *GetDXLDirectDispatchInfo() const
			{
				return m_direct_dispatch_info;
			}
			
			// needs the data to be sorted or not
			BOOL FInputSorted() const
			{
				return m_fInputSorted;
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
			CDXLPhysicalDML *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalDML == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLPhysicalDML*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLPhysicalDML_H

// EOF
