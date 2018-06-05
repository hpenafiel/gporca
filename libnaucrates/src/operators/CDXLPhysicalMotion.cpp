//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLPhysicalMotion.cpp
//
//	@doc:
//		Implementation of DXL physical motion operators
//---------------------------------------------------------------------------

#include "naucrates/dxl/operators/CDXLPhysicalMotion.h"
#include "naucrates/dxl/operators/CDXLNode.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpdxl;

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::CDXLPhysicalMotion
//
//	@doc:
//		Constructor
//
//---------------------------------------------------------------------------
CDXLPhysicalMotion::CDXLPhysicalMotion
	(
	IMemoryPool *memory_pool
	)
	:
	CDXLPhysical(memory_pool),
	m_input_segids_array(NULL),
	m_output_segids_array(NULL)
{
}

CDXLPhysicalMotion::~CDXLPhysicalMotion()
{
	CRefCount::SafeRelease(m_input_segids_array);
	CRefCount::SafeRelease(m_output_segids_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::GetInputSegIdsArray
//
//	@doc:
//		
//
//---------------------------------------------------------------------------
const IntPtrArray *
CDXLPhysicalMotion::GetInputSegIdsArray() const
{
	return m_input_segids_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::GetOutputSegIdsArray
//
//	@doc:
//		
//
//---------------------------------------------------------------------------
const IntPtrArray *
CDXLPhysicalMotion::GetOutputSegIdsArray() const
{
	return m_output_segids_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::SetInputSegIds
//
//	@doc:
//		
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMotion::SetInputSegIds(IntPtrArray *input_segids_array)
{
	GPOS_ASSERT(NULL == m_input_segids_array);
	GPOS_ASSERT(NULL != input_segids_array);
	m_input_segids_array = input_segids_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::SetOutputSegIds
//
//	@doc:
//		
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMotion::SetOutputSegIds(IntPtrArray *output_segids_array)
{
	GPOS_ASSERT(NULL == m_output_segids_array);
	GPOS_ASSERT(NULL != output_segids_array);
	m_output_segids_array = output_segids_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::SetSegmentInfo
//
//	@doc:
//		Set input and output segment information
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMotion::SetSegmentInfo
	(
	IntPtrArray *input_segids_array, 
	IntPtrArray *output_segids_array
	)
{
	GPOS_ASSERT(NULL == m_output_segids_array && NULL == m_input_segids_array);
	GPOS_ASSERT(NULL != output_segids_array && NULL != input_segids_array);
	
	m_input_segids_array = input_segids_array;
	m_output_segids_array = output_segids_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::GetSegIdsCommaSeparatedStr
//
//	@doc:
//		Serialize the array of segment ids into a comma-separated string		
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLPhysicalMotion::GetSegIdsCommaSeparatedStr(const IntPtrArray *pdrgpi) const
{
	GPOS_ASSERT(pdrgpi != NULL && 0 < pdrgpi->Size());
	
	CWStringDynamic *pstr = GPOS_NEW(m_memory_pool) CWStringDynamic(m_memory_pool);
	
	ULONG ulNumSegments = pdrgpi->Size();
	for (ULONG ul = 0; ul < ulNumSegments; ul++)
	{
		INT iSegId = *((*pdrgpi)[ul]);
		if (ul == ulNumSegments - 1)
		{
			// last element: do not print a comma
			pstr->AppendFormat(GPOS_WSZ_LIT("%d"), iSegId);
		}
		else
		{
			pstr->AppendFormat(GPOS_WSZ_LIT("%d%ls"), iSegId, CDXLTokens::PstrToken(EdxltokenComma)->GetBuffer());
		}
	}
	
	return pstr;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::GetInputSegIdsStr
//
//	@doc:
//		Serialize the array of input segment ids into a comma-separated string		
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLPhysicalMotion::GetInputSegIdsStr() const
{	
	return GetSegIdsCommaSeparatedStr(m_input_segids_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::GetOutputSegIdsStr
//
//	@doc:
//		Serialize the array of output segment ids into a comma-separated string		
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLPhysicalMotion::GetOutputSegIdsStr() const
{	
	return GetSegIdsCommaSeparatedStr(m_output_segids_array);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLPhysicalMotion::SerializeSegmentInfoToDXL
//
//	@doc:
//		Serialize the array of input and output segment ids in DXL format		
//
//---------------------------------------------------------------------------
void
CDXLPhysicalMotion::SerializeSegmentInfoToDXL
	(
	CXMLSerializer *xml_serializer
	) const
{
	CWStringDynamic *pstrInputSegIds = GetInputSegIdsStr();
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenInputSegments), pstrInputSegIds);
	
	CWStringDynamic *pstrOutputSegIds = GetOutputSegIdsStr();
	xml_serializer->AddAttribute(CDXLTokens::PstrToken(EdxltokenOutputSegments), pstrOutputSegIds);
		
	GPOS_DELETE(pstrInputSegIds);
	GPOS_DELETE(pstrOutputSegIds);
}


// EOF
