//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal, Inc.
//
//	Class for representing DXL Part List Values expressions
//	These expressions indicate the constant values for the list partition
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarPartListValues_H
#define GPDXL_CDXLScalarPartListValues_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpdxl
{

	class CDXLScalarPartListValues : public CDXLScalar
	{
		private:
			// partitioning level
			ULONG m_ulLevel;

			// result type
			IMDId *m_pmdidResult;

			// element type
			IMDId *m_pmdidElement;

			// private copy ctor
			CDXLScalarPartListValues(const CDXLScalarPartListValues&);

		public:
			// ctor
			CDXLScalarPartListValues(IMemoryPool *memory_pool, ULONG ulLevel, IMDId *pmdidResult, IMDId *pmdidElement);

			// dtor
			virtual
			~CDXLScalarPartListValues();

			// operator type
			virtual
			Edxlopid GetDXLOperator() const;

			// operator name
			virtual
			const CWStringConst *GetOpNameStr() const;

			// partitioning level
			ULONG UlLevel() const;

			// result type
			IMDId *PmdidResult() const;

			// element type
			IMDId *PmdidElement() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *dxlnode) const;

			// does the operator return a boolean result
			virtual
			BOOL HasBoolResult(CMDAccessor *md_accessor) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			virtual
			void AssertValid(const CDXLNode *dxlnode, BOOL validate_children) const;
#endif // GPOS_DEBUG

			// conversion function
			static
			CDXLScalarPartListValues *Cast(CDXLOperator *dxl_op);
	};
}

#endif // !GPDXL_CDXLScalarPartListValues_H

// EOF
