//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CDXLLogicalProject.h
//
//	@doc:
//		Class for representing DXL logical project operators
//		
//---------------------------------------------------------------------------
#ifndef GPDXL_CDXLLogicalProject_H
#define GPDXL_CDXLLogicalProject_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogical.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalProject
	//
	//	@doc:
	//		Class for representing DXL logical project operators
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalProject : public CDXLLogical
	{
		private:

			// private copy ctor
			CDXLLogicalProject(CDXLLogicalProject&);

			// alias name
			const CMDName *m_pmdnameAlias;

		public:
			// ctor
			explicit
			CDXLLogicalProject(IMemoryPool *);

			// accessors
			Edxlopid Edxlop() const;
			const CWStringConst *PstrOpName() const;
			const CMDName *MdName() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// set alias name
			void SetAliasName(CMDName *);

			// conversion function
			static
			CDXLLogicalProject *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopLogicalProject == dxl_op->Edxlop());

				return dynamic_cast<CDXLLogicalProject*>(dxl_op);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL validate_children) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLLogicalProject_H

// EOF
