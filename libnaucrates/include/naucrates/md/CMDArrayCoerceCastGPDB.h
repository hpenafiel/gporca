//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2017 Pivotal Software, Inc.
//
//	@filename:
//		CMDArrayCoerceCastGPDB.h
//
//	@doc:
//		Implementation of GPDB-specific array coerce cast functions in the
//		metadata cache
//---------------------------------------------------------------------------


#ifndef GPMD_CMDArrayCoerceCastGPDB_H
#define GPMD_CMDArrayCoerceCastGPDB_H

#include "gpos/base.h"

#include "naucrates/md/CMDCastGPDB.h"
#include "naucrates/dxl/operators/CDXLScalar.h"

namespace gpmd
{

	using namespace gpdxl;

	class CMDArrayCoerceCastGPDB : public CMDCastGPDB
	{
		private:
			// DXL for object
			const CWStringDynamic *m_pstr;

			// type mod
			INT m_type_modifier;

			// is explicit
			BOOL m_fIsExplicit;

			// CoercionForm
			EdxlCoercionForm m_edxlcf;

			// iLoc
			INT m_iLoc;

			// private copy ctor
			CMDArrayCoerceCastGPDB(const CMDArrayCoerceCastGPDB &);

		public:
			// ctor
			CMDArrayCoerceCastGPDB
				(
				IMemoryPool *memory_pool,
				IMDId *mdid,
				CMDName *mdname,
				IMDId *mdid_src,
				IMDId *mdid_dest,
				BOOL is_binary_coercible,
				IMDId *mdid_cast_func,
				EmdCoercepathType emdPathType,
				INT type_modifier,
				BOOL is_explicit,
				EdxlCoercionForm edxlcf,
				INT iLoc
				);

			// dtor
			virtual
			~CMDArrayCoerceCastGPDB();

			// accessors
			virtual
			const CWStringDynamic *Pstr() const
			{
				return m_pstr;
			}

			// return type modifier
			virtual
			INT TypeModifier() const;

			virtual
			BOOL FIsExplicit() const;

			// return coercion form
			virtual
			EdxlCoercionForm Ecf() const;

			// return token location
			virtual
			INT ILoc() const;

			// serialize object in DXL format
			virtual
			void Serialize(gpdxl::CXMLSerializer *xml_serializer) const;

#ifdef GPOS_DEBUG
			// debug print of the type in the provided stream
			virtual
			void DebugPrint(IOstream &os) const;
#endif
	};
}

#endif // !GPMD_CMDArrayCoerceCastGPDB_H

// EOF
