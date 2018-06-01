//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryAll.cpp
//
//	@doc:
//		Implementation of subquery ALL
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLScalarSubqueryAll.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryAll::CDXLScalarSubqueryAll
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryAll::CDXLScalarSubqueryAll
	(
	IMemoryPool *memory_pool,
	IMDId *pmdidScalarOp,
	CMDName *pmdnameScalarOp,
	ULONG ulColId
	)
	:
	CDXLScalarSubqueryQuantified(memory_pool, pmdidScalarOp, pmdnameScalarOp, ulColId)
{}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryAll::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSubqueryAll::GetDXLOperator() const
{
	return EdxlopScalarSubqueryAll;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryAll::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubqueryAll::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSubqueryAll);
}

// EOF
