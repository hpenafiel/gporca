//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalTVF.h
//
//	@doc:
//		Class for representing DXL physical table-valued functions
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLPhysicalTVF_H
#define GPDXL_CDXLPhysicalTVF_H

#include "gpos/base.h"
#include "naucrates/md/IMDId.h"
#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLPhysicalTVF
	//
	//	@doc:
	//		Class for representing DXL physical table-valued functions
	//
	//---------------------------------------------------------------------------
	class CDXLPhysicalTVF : public CDXLPhysical
	{
		private:
			// function mdid
			IMDId *m_pmdidFunc;

			// return type
			IMDId *m_pmdidRetType;

			// function name
			CWStringConst *m_pstr;

			// private copy ctor
			CDXLPhysicalTVF(const CDXLPhysicalTVF &);

		public:
			// ctor
			CDXLPhysicalTVF
				(
					IMemoryPool *memory_pool,
					IMDId *pmdidFunc,
					IMDId *pmdidRetType,
					CWStringConst *pstr
				);

			// dtor
			virtual
			~CDXLPhysicalTVF();

			// get operator type
			Edxlopid Edxlop() const;

			// get operator name
			const CWStringConst *PstrOpName() const;

			// get function name
			CWStringConst *Pstr() const
			{
				return m_pstr;
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

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLPhysicalTVF *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopPhysicalTVF == dxl_op->Edxlop());

				return dynamic_cast<CDXLPhysicalTVF*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}

#endif // !GPDXL_CDXLPhysicalTVF_H

// EOF

