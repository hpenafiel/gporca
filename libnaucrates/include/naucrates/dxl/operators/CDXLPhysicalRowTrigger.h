//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalRowTrigger.h
//
//	@doc:
//		Class for representing physical row trigger operator
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalRowTrigger_H
#define GPDXL_CDXLPhysicalRowTrigger_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"

#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/md/IMDId.h"

using gpmd::IMDId;

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalRowTrigger
	//
	//	@doc:
	//		Class for representing physical row trigger operators
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalRowTrigger : public CDXLPhysical
	{
		private:

			// relation id on which triggers are to be executed
			IMDId *m_rel_mdid;

			// trigger type
			INT m_iType;

			// old column ids
			ULongPtrArray *m_pdrgpulOld;

			// new column ids
			ULongPtrArray *m_pdrgpulNew;

			// private copy ctor
			CDXLPhysicalRowTrigger(const CDXLPhysicalRowTrigger &);

		public:

			// ctor
			CDXLPhysicalRowTrigger
				(
				IMemoryPool *memory_pool,
				IMDId *pmdidRel,
				INT iType,
				ULongPtrArray *pdrgpulOld,
				ULongPtrArray *pdrgpulNew
				);

			// dtor
			virtual
			~CDXLPhysicalRowTrigger();

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// relation id
			IMDId *GetRelMdId() const
			{
				return m_rel_mdid;
			}

			// trigger type
			INT IType() const
			{
				return m_iType;
			}

			// old column ids
			ULongPtrArray *PdrgpulOld() const
			{
				return m_pdrgpulOld;
			}

			// new column ids
			ULongPtrArray *PdrgpulNew() const
			{
				return m_pdrgpulNew;
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *dxlnode, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *dxlnode) const;

			// conversion function
			static
			CDXLPhysicalRowTrigger *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalRowTrigger == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLPhysicalRowTrigger*>(dxl_op);
			}
	};
}

#endif // !GPDXL_CDXLPhysicalRowTrigger_H

// EOF
