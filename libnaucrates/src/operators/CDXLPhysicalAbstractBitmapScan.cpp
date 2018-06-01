//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CDXLPhysicalAbstractBitmapScan.cpp
//
//	@doc:
//		Parent class for representing DXL bitmap table scan operators, both
//		not partitioned and dynamic.
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalAbstractBitmapScan.h"

#include "naucrates/dxl/operators/CDXLTableDescr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "naucrates/dxl/xml/dxltokens.h"

using namespace gpdxl;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAbstractBitmapScan::~CDXLPhysicalAbstractBitmapScan
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysicalAbstractBitmapScan::~CDXLPhysicalAbstractBitmapScan()
{
	m_pdxltabdesc->Release();
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalAbstractBitmapScan::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured
//
//---------------------------------------------------------------------------
void
CDXLPhysicalAbstractBitmapScan::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	)
	const
{
	GPOS_ASSERT(4 == pdxln->Arity());

	// assert proj list and filter are valid
	CDXLPhysical::AssertValid(pdxln, validate_children);

	GPOS_ASSERT(EdxlopScalarRecheckCondFilter == (*pdxln)[2]->GetOperator()->Edxlop());

	// assert bitmap access path is valid
	CDXLNode *pdxlnBitmap = (*pdxln)[3];
	GPOS_ASSERT(EdxlopScalarBitmapIndexProbe == pdxlnBitmap->GetOperator()->Edxlop() ||
			EdxlopScalarBitmapBoolOp == pdxlnBitmap->GetOperator()->Edxlop());

	// assert validity of table descriptor
	GPOS_ASSERT(NULL != m_pdxltabdesc);
	GPOS_ASSERT(NULL != m_pdxltabdesc->MdName());
	GPOS_ASSERT(m_pdxltabdesc->MdName()->Pstr()->IsValid());

	if (validate_children)
	{
		pdxlnBitmap->GetOperator()->AssertValid(pdxlnBitmap, validate_children);
	}
}
#endif // GPOS_DEBUG

// EOF
