//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CDXLLogicalConstTable.h
//
//	@doc:
//		Class for representing DXL logical constant tables
//		
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLLogicalConstTable_H
#define GPDXL_CDXLLogicalConstTable_H

#include "gpos/base.h"
#include "naucrates/dxl/operators/CDXLLogical.h"
#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/operators/CDXLDatum.h"

namespace gpdxl
{
	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLLogicalConstTable
	//
	//	@doc:
	//		Class for representing DXL logical const table operators
	//
	//---------------------------------------------------------------------------
	class CDXLLogicalConstTable : public CDXLLogical
	{
		private:

			// list of column descriptors		
			ColumnDescrDXLArray *m_pdrgpdxlcd;

			// array of datum arrays (const tuples)
			DXLDatumArrays *m_pdrgpdrgpdxldatum;

			// private copy ctor
			CDXLLogicalConstTable(CDXLLogicalConstTable&);

		public:
			// ctor
			CDXLLogicalConstTable
				(
				IMemoryPool *memory_pool,
				ColumnDescrDXLArray *pdrgpdxlcd,
				DXLDatumArrays *pdrgpdrgpdxldatum
				);

			//dtor
			virtual
			~CDXLLogicalConstTable();

			// accessors

			// operator type
			Edxlopid Edxlop() const;

			// operator name
			const CWStringConst *PstrOpName() const;

			// column descriptors
			const ColumnDescrDXLArray *GetColumnDescrDXLArray() const
			{
				return m_pdrgpdxlcd;
			}

			// return the column descriptor at a given position
			CDXLColDescr *GetColumnDescrAt(ULONG ul) const;

			// number of columns
			ULONG Arity() const;

			// number of constant tuples
			ULONG UlTupleCount() const
			{
				return m_pdrgpdrgpdxldatum->Size();
			}

			// return the const tuple (datum array) at a given position
			const DXLDatumArray *PrgPdxldatumConstTuple(ULONG ulTuplePos) const
			{
				return (*m_pdrgpdrgpdxldatum)[ulTuplePos];
			}

			// serialize operator in DXL format
			virtual
			void SerializeToDXL(CXMLSerializer *xml_serializer, const CDXLNode *pdxln) const;

			// check if given column is defined by operator
			virtual
			BOOL FDefinesColumn(ULONG ulColId) const;

			// conversion function
			static
			CDXLLogicalConstTable *PdxlopConvert
				(
				CDXLOperator *pdxlop
				)
			{
				GPOS_ASSERT(NULL != pdxlop);
				GPOS_ASSERT(EdxlopLogicalConstTable == pdxlop->Edxlop());

				return dynamic_cast<CDXLLogicalConstTable*>(pdxlop);
			}

#ifdef GPOS_DEBUG
			// checks whether the operator has valid structure, i.e. number and
			// types of child nodes
			void AssertValid(const CDXLNode *, BOOL fValidateChildren) const;
#endif // GPOS_DEBUG

	};
}
#endif // !GPDXL_CDXLLogicalConstTable_H

// EOF
