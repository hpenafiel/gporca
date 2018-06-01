//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysical.cpp
//
//	@doc:
//		Implementation of DXL physical operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysical.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysical::CDXLPhysical
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CDXLPhysical::CDXLPhysical
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLOperator(memory_pool)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysical::~CDXLPhysical
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CDXLPhysical::~CDXLPhysical()
{
}

//---------------------------------------------------------------------------
//      @function:
//              CDXLPhysical::Edxloperatortype
//
//      @doc:
//              Operator Type
//
//---------------------------------------------------------------------------
Edxloptype
CDXLPhysical::Edxloperatortype() const
{
	return EdxloptypePhysical;
}


#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysical::AssertValid
//
//	@doc:
//		Checks whether operator node is well-structured 
//
//---------------------------------------------------------------------------
void
CDXLPhysical::AssertValid
	(
	const CDXLNode *pdxln,
	BOOL validate_children
	) const
{
	GPOS_ASSERT(NULL != pdxln);
	
	GPOS_ASSERT(2 <= pdxln->Arity());
	
	CDXLNode *pdxlnProjList = (*pdxln)[0];
	CDXLNode *pdxlnFilter = (*pdxln)[1];
	
	GPOS_ASSERT(EdxlopScalarProjectList == pdxlnProjList->GetOperator()->Edxlop());
	GPOS_ASSERT(EdxlopScalarFilter == pdxlnFilter->GetOperator()->Edxlop());
	
	if (validate_children)
	{
		pdxlnProjList->GetOperator()->AssertValid(pdxlnProjList, validate_children);
		pdxlnFilter->GetOperator()->AssertValid(pdxlnFilter, validate_children);
	}
}
#endif // GPOS_DEBUG


// EOF

