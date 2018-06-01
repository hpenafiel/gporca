//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarIdent.h
//
//	@doc:
//		Class for representing DXL scalar identifier operators.
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarIdent_H
#define GPDXL_CDXLScalarIdent_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/dxl/operators/CDXLColRef.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpmd;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarIdent
	//
	//	@doc:
	//		Class for representing DXL scalar identifier operators.
	//
	//---------------------------------------------------------------------------
	class CDXLScalarIdent : public CDXLScalar
	{
		private:
			// column reference
			CDXLColRef *m_pdxlcr;
			
			// private copy ctor
			CDXLScalarIdent(CDXLScalarIdent&);
			
		public:
			// ctor/dtor
			CDXLScalarIdent
				(
				IMemoryPool *,
				CDXLColRef *
				);
			
			virtual
			~CDXLScalarIdent();
			
			// ident accessors
			Edxlopid Edxlop() const;
			
			// name of the operator
			const CWStringConst *PstrOpName() const;

			// accessors
			const CDXLColRef *Pdxlcr() const;

			IMDId *MDIdType() const;

			INT TypeModifier() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarIdent *PdxlopConvert
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarIdent == dxl_op->Edxlop());

				return dynamic_cast<CDXLScalarIdent*>(dxl_op);
			}
			
			// does the operator return a boolean result
			virtual
			BOOL FBoolean(CMDAccessor *pmda) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
			
	};
}



#endif // !GPDXL_CDXLScalarIdent_H

// EOF

