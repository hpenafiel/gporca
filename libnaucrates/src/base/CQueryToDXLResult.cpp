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
	DXLNodeArray *pdrgpdxlnOutput,
	DXLNodeArray *cte_dxlnode_array
	)
	:
	m_dxl_node(pdxln),
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
	m_dxl_node->Release();
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
const DXLNodeArray*
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
const DXLNodeArray*
CQueryToDXLResult::GetCTEProducerDXLArray() const
{
	return m_cte_producer_dxl_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CQueryToDXLResult::CreateDXLNode
//
//	@doc:
//		Return the DXL node representing the query
//
//---------------------------------------------------------------------------
const CDXLNode *
CQueryToDXLResult::CreateDXLNode() const
{
	return m_dxl_node;
}



// EOF
