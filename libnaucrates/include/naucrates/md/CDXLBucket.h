//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLBucket.h
//
//	@doc:
//		Class representing buckets in a DXL column stats histogram
//---------------------------------------------------------------------------



#ifndef GPMD_CDXLBucket_H
#define GPMD_CDXLBucket_H

#include "gpos/base.h"
#include "gpos/common/CDouble.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "naucrates/dxl/operators/CDXLDatum.h"

namespace gpdxl
{
	class CXMLSerializer;
}

namespace gpmd
{
	using namespace gpos;
	using namespace gpdxl;

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLBucket
	//
	//	@doc:
	//		Class representing a bucket in DXL column stats
	//
	//---------------------------------------------------------------------------
	class CDXLBucket : public CRefCount
	{		
		private:
		
			// lower bound value for the bucket
			CDXLDatum *m_lower_bound_datum_dxl;
			
			// max value for the bucket
			CDXLDatum *m_upper_bound_datum_dxl;

			// is lower bound closed (i.e., the boundary point is included in the bucket)
			BOOL m_is_lower_closed;

			// is upper bound closed (i.e., the boundary point is included in the bucket)
			BOOL m_is_upper_closed;

			// frequency
			CDouble m_frequency;
			
			// distinct values
			CDouble m_distinct;

			// private copy ctor
			CDXLBucket(const CDXLBucket &);
		
			// serialize the bucket boundary
			void SerializeBoundaryValue(CXMLSerializer *xml_serializer, const CWStringConst *pstrElem, CDXLDatum *datum_dxl, BOOL fBoundClosed) const;

		public:
			
			// ctor
			CDXLBucket
				(
				CDXLDatum *pdatumLower,
				CDXLDatum *pdatumUpper,
				BOOL fLowerClosed,
				BOOL fUpperClosed,
				CDouble dFrequency,
				CDouble dDistinct
				);
			
			// dtor
			virtual
			~CDXLBucket();
			
			// is lower bound closed
			BOOL FLowerClosed() const
			{
				return m_is_lower_closed;
			}

			// is upper bound closed
			BOOL FUpperClosed() const
			{
				return m_is_upper_closed;
			}

			// min value for the bucket
			const CDXLDatum *PdxldatumLower() const;
			
			// max value for the bucket
			const CDXLDatum *PdxldatumUpper() const;
						
			// frequency
			CDouble DFrequency() const;
			
			// distinct values
			CDouble DDistinct() const;

			// serialize bucket in DXL format
			void Serialize(gpdxl::CXMLSerializer *) const;

#ifdef GPOS_DEBUG
			// debug print of the bucket
			void DebugPrint(IOstream &os) const;
#endif
			
	};

	// array of dxl buckets
	typedef CDynamicPtrArray<CDXLBucket, CleanupRelease> DrgPdxlbucket;
}

#endif // !GPMD_CDXLBucket_H

// EOF
