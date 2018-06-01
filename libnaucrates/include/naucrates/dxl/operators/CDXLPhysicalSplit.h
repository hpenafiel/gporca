//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalSplit.h
//
//	@doc:
//		Class for representing physical split operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalSplit_H
#define GPDXL_CDXLPhysicalSplit_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"

namespace gpdxl
{
	// fwd decl
	class CDXLTableDescr;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalSplit
	//
	//	@doc:
	//		Class for representing physical split operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalSplit : public CDXLPhysical
	{
		private:

			// list of deletion column ids
			ULongPtrArray *m_pdrgpulDelete;

			// list of insertion column ids
			ULongPtrArray *m_pdrgpulInsert;

			// action column id
			ULONG m_ulAction;

			// ctid column id
			ULONG m_ulCtid;

			// segmentid column id
			ULONG m_ulSegmentId;

			// should update preserve tuple oids
			BOOL m_fPreserveOids;	

			// tuple oid column id
			ULONG m_ulTupleOid;
			
			// private copy ctor
			CDXLPhysicalSplit(const CDXLPhysicalSplit &);

		public:

			// ctor
			CDXLPhysicalSplit
				(
				IMemoryPool *memory_pool,
				ULongPtrArray *pdrgpulDelete,
				ULongPtrArray *pdrgpulInsert,
				ULONG ulAction,
				ULONG ulCtid,
				ULONG ulSegmentId,
				BOOL fPreserveOids,
				ULONG ulTupleOid
				);

			// dtor
			virtual
			~CDXLPhysicalSplit();

			// operator type
			Edxlopid Edxlop() const;

			// operator name
			const CWStringConst *PstrOpName() const;

			// deletion column ids
			ULongPtrArray *PdrgpulDelete() const
			{
				return m_pdrgpulDelete;
			}

			// insertion column ids
			ULongPtrArray *PdrgpulInsert() const
			{
				return m_pdrgpulInsert;
			}

			// action column id
			ULONG UlAction() const
			{
				return m_ulAction;
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
			CDXLPhysicalSplit *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalSplit == dxl_op->Edxlop());

				return dynamic_cast<CDXLPhysicalSplit*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLPhysicalSplit_H

// EOF
