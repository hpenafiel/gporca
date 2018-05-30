//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC, Corp.
//
//	@filename:
//		CDXLScalarSubqueryAny.cpp
//
//	@doc:
//		Implementation of subquery ANY
//---------------------------------------------------------------------------

#include "gpos/string/CWStringDynamic.h"

#include "naucrates/dxl/operators/CDXLScalarSubqueryAny.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryAny::CDXLScalarSubqueryAny
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarSubqueryAny::CDXLScalarSubqueryAny
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
//		CDXLScalarSubqueryAny::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarSubqueryAny::Edxlop() const
{
	return EdxlopScalarSubqueryAny;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarSubqueryAny::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarSubqueryAny::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarSubqueryAny);
}

// EOF
