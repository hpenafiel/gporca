//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLScalarOneTimeFilter.cpp
//
//	@doc:
//		Implementation of DXL physical one-time filter operator
//---------------------------------------------------------------------------


#include "naucrates/dxl/operators/CDXLScalarOneTimeFilter.h"

#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOneTimeFilter::CDXLScalarOneTimeFilter
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLScalarOneTimeFilter::CDXLScalarOneTimeFilter
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLScalarFilter(memory_pool)
{
}



//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOneTimeFilter::Edxlop
//
//	@doc:
//		Operator type
//
//---------------------------------------------------------------------------
Edxlopid
CDXLScalarOneTimeFilter::Edxlop() const
{
	return EdxlopScalarOneTimeFilter;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOneTimeFilter::PstrOpName
//
//	@doc:
//		Operator name
//
//---------------------------------------------------------------------------
const CWStringConst *
CDXLScalarOneTimeFilter::PstrOpName() const
{
	return CDXLTokens::PstrToken(EdxltokenScalarOneTimeFilter);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLScalarOneTimeFilter::SerializeToDXL
//
//	@doc:
//		Serialize operator in DXL format
//
//---------------------------------------------------------------------------
void
CDXLScalarOneTimeFilter::SerializeToDXL
	(
	CXMLSerializer *xml_serializer,
	const CDXLNode * pdxln
	)
	const
{
	const CWStringConst *element_name = PstrOpName();

	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);

	// serilize children
	pdxln->SerializeChildrenToDXL(xml_serializer);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), element_name);
}



// EOF
