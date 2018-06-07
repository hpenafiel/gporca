//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CParseHandlerMDRelation.h
//
//	@doc:
//		SAX parse handler class for relation metadata
//---------------------------------------------------------------------------

#ifndef GPDXL_CParseHandlerMDRelation_H
#define GPDXL_CParseHandlerMDRelation_H

#include "gpos/base.h"
#include "naucrates/dxl/parser/CParseHandlerMetadataObject.h"
#include "naucrates/dxl/xml/dxltokens.h"

#include "naucrates/md/CMDRelationGPDB.h"
#include "naucrates/md/CMDPartConstraintGPDB.h"

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;

	XERCES_CPP_NAMESPACE_USE
	
	//---------------------------------------------------------------------------
	//	@class:
	//		CParseHandlerMDRelation
	//
	//	@doc:
	//		Parse handler for relation metadata
	//
	//---------------------------------------------------------------------------
	class CParseHandlerMDRelation : public CParseHandlerMetadataObject
	{
		protected:
			// id and version of the relation
			IMDId *m_mdid;
			
			// schema name
			CMDName *m_mdname_schema;
			
			// table name
			CMDName *m_mdname;
			
			// is this a temporary relation
			BOOL m_is_temp_table;
			
			// does this relation have oids
			BOOL m_has_oids;

			// storage type
			IMDRelation::Erelstoragetype m_rel_storage_type;
			
			// distribution policy
			IMDRelation::GetRelDistrPolicy m_rel_distr_policy;
			
			// distribution columns
			ULongPtrArray *m_pdrgpulDistrColumns;
			
			// do we need to consider a hash distributed table as random distributed
			BOOL m_fConvertHashToRandom;
			
			// partition keys
			ULongPtrArray *m_pdrgpulPartColumns;

			// partition types
			CharPtrArray *m_pdrgpszPartTypes;

			// number of partitions
			ULONG m_ulPartitions;
			
			// key sets
			ULongPtrArray2D *m_pdrgpdrgpulKeys;

			// part constraint
			CMDPartConstraintGPDB *m_ppartcnstr;
			
			// levels that include default partitions
			ULongPtrArray *m_pdrgpulDefaultParts;

			// is part constraint unbounded
			BOOL m_fPartConstraintUnbounded; 
			
			// helper function to parse main relation attributes: name, id,
			// distribution policy and keys
			void ParseRelationAttributes(const Attributes& attrs, Edxltoken edxltokenElement);

			// create and activate the parse handler for the children nodes
			void ParseChildNodes();

		private:
			// private copy ctor
			CParseHandlerMDRelation(const CParseHandlerMDRelation &);
			
			// process the start of an element
			void StartElement
				(
					const XMLCh* const element_uri, 		// URI of element's namespace
 					const XMLCh* const element_local_name,	// local part of element's name
					const XMLCh* const element_qname,		// element's qname
					const Attributes& attr				// element's attributes
				);
				
			// process the end of an element
			void EndElement
				(
					const XMLCh* const element_uri, 		// URI of element's namespace
					const XMLCh* const element_local_name,	// local part of element's name
					const XMLCh* const element_qname		// element's qname
				);		
			
		public:
			// ctor
			CParseHandlerMDRelation
				(
				IMemoryPool *memory_pool,
				CParseHandlerManager *parse_handler_mgr,
				CParseHandlerBase *parse_handler_root
				);			
	};
}

#endif // !GPDXL_CParseHandlerMDRelation_H

// EOF
