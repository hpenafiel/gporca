//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CQueryToDXLResult.cpp
//
//	@doc:
//		Implementation of the methods for accessing the result of the translation
//---------------------------------------------------------------------------


#include "naucrates/base/CQueryToDXLResult.h"


using namespace gpdxl;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::CQueryToDXLResult
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CQueryToDXLResult::CQueryToDXLResult
	(
	CDXLNode *pdxln,
	DrgPdxln *pdrgpdxlnOutput,
	DrgPdxln *cte_dxlnode_array
	)
	:
	m_pdxln(pdxln),
	m_pdrgpdxlnQueryOutput(pdrgpdxlnOutput),
	m_cte_producer_dxl_array(cte_dxlnode_array)
{
	GPOS_ASSERT(NULL != pdxln);
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::~CQueryToDXLResult
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CQueryToDXLResult::~CQueryToDXLResult()
{
	m_pdxln->Release();
	CRefCount::SafeRelease(m_pdrgpdxlnQueryOutput);
	CRefCount::SafeRelease(m_cte_producer_dxl_array);

}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::GetOutputColumnsDXLArray
//
//	@doc:
//		Return the array of dxl nodes representing the query output
//
//---------------------------------------------------------------------------
const DrgPdxln*
CQueryToDXLResult::GetOutputColumnsDXLArray() const
{
	return m_pdrgpdxlnQueryOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::GetCTEProducerDXLArray
//
//	@doc:
//		Return the array of CTEs
//
//---------------------------------------------------------------------------
const DrgPdxln*
CQueryToDXLResult::GetCTEProducerDXLArray() const
{
	return m_cte_producer_dxl_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::Pdxln
//
//	@doc:
//		Return the DXL node representing the query
//
//---------------------------------------------------------------------------
const CDXLNode *
CQueryToDXLResult::Pdxln() const
{
	return m_pdxln;
}



// EOF
