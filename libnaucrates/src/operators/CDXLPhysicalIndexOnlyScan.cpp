//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLPhysicalIndexOnlyScan.cpp
//
//	@doc:
//		Implementation of DXL physical index only scan operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalIndexOnlyScan.h"

#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexOnlyScan::CDXLPhysicalIndexOnlyScan
//
//	@doc:
//		Construct an index only scan node given its table descriptor,
//		index descriptor and filter conditions on the index
//
//---------------------------------------------------------------------------
CDXLPhysicalIndexOnlyScan::CDXLPhysicalIndexOnlyScan
	(
	IMemoryPool *memory_pool,
	CDXLTableDescr *table_descr,
	CDXLIndexDescr *pdxlid,
	EdxlIndexScanDirection edxlisd
	)
	:
	CDXLPhysicalIndexScan(memory_pool, table_descr, pdxlid, edxlisd)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexOnlyScan::GetDXLOperator
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLPhysicalIndexOnlyScan::GetDXLOperator() const
{
	return EdxlopPhysicalIndexOnlyScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalIndexOnlyScan::GetOpNameStr
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLPhysicalIndexOnlyScan::GetOpNameStr() const
{
	return CDXLTokens::PstrToken(EdxltokenPhysicalIndexOnlyScan);
}

// EOF
