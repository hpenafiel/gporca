//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLScalarWindowRef.h
//
//	@doc:
//		Class for representing DXL scalar WindowRef
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLScalarWindowRef_H
#define GPDXL_CDXLScalarWindowRef_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	// stage of the evaluation of the window function
	enum EdxlWinStage
	{
		EdxlwinstageImmediate = 0,
		EdxlwinstagePreliminary,
		EdxlwinstageRowKey,
		EdxlwinstageSentinel
	};

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLScalarWindowRef
	//
	//	@doc:
	//		Class for representing DXL scalar WindowRef
	//
	//---------------------------------------------------------------------------
	class CDXLScalarWindowRef : public CDXLScalar
	{
		private:

			// catalog id of the function
			IMDId *m_func_mdid;

			// return type
			IMDId *m_return_type_mdid;

			// denotes whether it's agg(DISTINCT ...)
			BOOL m_fDistinct;

			// is argument list really '*' //
			BOOL m_fStarArg;

			// is function a simple aggregate? //
			BOOL m_fSimpleAgg;

			// denotes the win stage
			EdxlWinStage m_edxlwinstage;

			// position the window specification in a parent window operator
			ULONG m_ulWinspecPos;

			// private copy ctor
			CDXLScalarWindowRef(const CDXLScalarWindowRef&);

		public:
			// ctor
			CDXLScalarWindowRef
				(
				IMemoryPool *memory_pool,
				IMDId *pmdidWinfunc,
				IMDId *mdid_return_type,
				BOOL fDistinct,
				BOOL fStarArg,
				BOOL fSimpleAgg,
				EdxlWinStage edxlwinstage,
				ULONG ulWinspecPosition
				);

			//dtor
			virtual
			~CDXLScalarWindowRef();

			// ident accessors
			Edxlopid GetDXLOperator() const;

			// name of the DXL operator
			const CWStringConst *GetOpNameStr() const;

			// catalog id of the function
			IMDId *FuncMdId() const
			{
				return m_func_mdid;
			}

			// return type of the function
			IMDId *ReturnTypeMdId() const
			{
				return m_return_type_mdid;
			}

			// window stage
			EdxlWinStage Edxlwinstage() const
			{
				return m_edxlwinstage;
			}

			// denotes whether it's agg(DISTINCT ...)
			BOOL FDistinct() const
			{
				return m_fDistinct;
			}
		
			BOOL FStarArg() const
			{
				return m_fStarArg;
			}

			BOOL FSimpleAgg() const
			{
				return m_fSimpleAgg;
			}

			// position the window specification in a parent window operator
			ULONG UlWinSpecPos() const
			{
				return m_ulWinspecPos;
			}

			// set window spec position
			void SetWinSpecPos
				(
				ULONG ulWinspecPos
				)
			{
				m_ulWinspecPos = ulWinspecPos;
			}

			// string representation of win stage
			const CWStringConst *PstrWinStage() const;

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// conversion function
			static
			CDXLScalarWindowRef *Cast
				(
				CDXLOperator *dxl_op
				)
			{
				GPOS_ASSERT(NULL != dxl_op);
				GPOS_ASSERT(EdxlopScalarWindowRef == dxl_op->GetDXLOperator());

				return dynamic_cast<CDXLScalarWindowRef*>(dxl_op);
			}

			// does the operator return a boolean result
			virtual
			BOOL HasBoolResult(CMDAccessor *md_accessor) const;

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *pdxln, BOOL validate_children) const;
#endif // GPOS_DEBUG
	};
}

#endif // !GPDXL_CDXLScalarWindowRef_H

// EOF

