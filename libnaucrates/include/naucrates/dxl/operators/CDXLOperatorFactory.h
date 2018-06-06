//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLOperatorFactory.h
//
//	@doc:
//		Factory for creating DXL tree elements out of parsed XML attributes
//---------------------------------------------------------------------------

#ifndef GPDXL_CDXLOperatorFactory_H
#define GPDXL_CDXLOperatorFactory_H

#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLStringTokenizer.hpp>

#include "gpos/base.h"
#include "naucrates/dxl/operators/dxlops.h"
#include "naucrates/dxl/xml/CDXLMemoryManager.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/md/IMDId.h"
#include "naucrates/md/CMDFunctionGPDB.h"
#include "naucrates/md/CMDRelationGPDB.h"
#include "naucrates/md/IMDIndex.h"
#include "naucrates/base/IDatum.h"
#include "gpos/common/CDouble.h"

// dynamic array of XML strings
typedef CDynamicPtrArray<XMLCh, CleanupNULL> DrgPxmlsz;

// fwd decl
namespace gpmd
{
	class CMDIdGPDB;
	class CMDIdColStats;
	class CMDIdRelStats;
	class CMDIdCast;
	class CMDIdScCmp;
}

namespace gpdxl
{
	using namespace gpos;
	using namespace gpmd;
	using namespace gpnaucrates;

	XERCES_CPP_NAMESPACE_USE
	
	//fwd decl
	class CDXLMemoryManager;
	class CDXLDatum;

	// shorthand for functions for translating a DXL datum
	typedef CDXLDatum* (PfPdxldatum) (CDXLMemoryManager *, const Attributes &, Edxltoken , IMDId *, BOOL , BOOL);

	//---------------------------------------------------------------------------
	//	@class:
	//		CDXLOperatorFactory
	//
	//	@doc:
	//		Factory class containing static methods for creating DXL objects
	//		from parsed DXL information such as XML element's attributes
	//
	//---------------------------------------------------------------------------
	class CDXLOperatorFactory
	{
			
		private:

			// return the LINT value of byte array
			static
			LINT Value
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attrs,
				Edxltoken edxltokenElement,
				BYTE *pba
				);

			// parses a byte array representation of the datum
			static
			BYTE *GetByteArray
					(
					CDXLMemoryManager *memory_manager_dxl,
					const Attributes &attrs,
					Edxltoken edxltokenElement,
					ULONG *pulLength
					);

		public:

			// pair of oid for datums and the factory function
			struct SDXLDatumFactoryElem
			{
				OID oid;
				PfPdxldatum *pf;
			};

			static
			CDXLDatum *PdxldatumOid
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			static
			CDXLDatum *PdxldatumInt2
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			static
			CDXLDatum *PdxldatumInt4
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			static
			CDXLDatum *PdxldatumInt8
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			static
			CDXLDatum *PdxldatumBool
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			// parse a dxl datum of type generic
			static
			CDXLDatum *PdxldatumGeneric
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			// parse a dxl datum of types that need double mapping
			static
			CDXLDatum *PdxldatumStatsDoubleMappable
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			// parse a dxl datum of types that need lint mapping
			static
			CDXLDatum *PdxldatumStatsLintMappable
								(
								CDXLMemoryManager *memory_manager_dxl,
								const Attributes &attrs,
								Edxltoken edxltokenElement,
								IMDId *pmdid,
								BOOL fConstNull ,
								BOOL fConstByVal
								);

			// create a table scan operator
			static
			CDXLPhysical *MakeDXLTblScan(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a subquery scan operator
			static
			CDXLPhysical *MakeDXLSubqScan(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a result operator
			static
			CDXLPhysical *MakeDXLResult(CDXLMemoryManager *memory_manager_dxl);

			// create a hashjoin operator
			static
			CDXLPhysical *MakeDXLHashJoin(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a nested loop join operator
			static
			CDXLPhysical *MakeDXLNLJoin(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a merge join operator
			static
			CDXLPhysical *MakeDXLMergeJoin(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a gather motion operator
			static
			CDXLPhysical *MakeDXLGatherMotion(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a broadcast motion operator
			static
			CDXLPhysical *MakeDXLBroadcastMotion(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a redistribute motion operator
			static
			CDXLPhysical *MakeDXLRedistributeMotion(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a routed motion operator
			static
			CDXLPhysical *MakeDXLRoutedMotion(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a random motion operator
			static
			CDXLPhysical *MakeDXLRandomMotion(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create an append operator
			static
			CDXLPhysical *MakeDXLAppend(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a limit operator
			static
			CDXLPhysical *MakeDXLLimit(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create an aggregation operator
			static
			CDXLPhysical *MakeDXLAgg(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a sort operator
			static
			CDXLPhysical *MakeDXLSort(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a materialize operator
			static
			CDXLPhysical *MakeDXLMaterialize(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a limit count operator
			static
			CDXLScalar *MakeDXLLimitCount(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a limit offset operator
			static
			CDXLScalar *MakeDXLLimitOffset(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a scalar comparison operator
			static
			CDXLScalar *MakeDXLScalarCmp(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a distinct comparison operator
			static
			CDXLScalar *MakeDXLDistinctCmp(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a scalar OpExpr
			static
			CDXLScalar *MakeDXLOpExpr(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a scalar ArrayComp
			static
			CDXLScalar *PdxlopArrayComp(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a BoolExpr
			static
			CDXLScalar *MakeDXLBoolExpr(CDXLMemoryManager *memory_manager_dxl, const EdxlBoolExprType);

			// create a boolean test
			static
			CDXLScalar *MakeDXLBooleanTest(CDXLMemoryManager *memory_manager_dxl, const EdxlBooleanTestType);

			// create a subplan operator
			static
			CDXLScalar *MakeDXLSubPlan
				(
				CDXLMemoryManager *memory_manager_dxl,
				IMDId *pmdid,
				DrgPdxlcr *pdrgdxlcr,
				EdxlSubPlanType edxlsubplantype,
				CDXLNode *pdxlnTestExpr
				);

			// create a NullTest
			static
			CDXLScalar *MakeDXLNullTest(CDXLMemoryManager *memory_manager_dxl, const BOOL );

			// create a cast
			static
			CDXLScalar *MakeDXLCast(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a coerce
			static
			CDXLScalar *MakeDXLCoerceToDomain(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a CoerceViaIo
			static
			CDXLScalar *MakeDXLCoerceViaIO(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a ArrayCoerceExpr
			static
			CDXLScalar *MakeDXLArrayCoerceExpr(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a scalar identifier operator
			static
			CDXLScalar *PdxlopScalarIdent(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a scalar Const
			static
			CDXLScalar *MakeDXLConstValue(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a CaseStmt
			static
			CDXLScalar *MakeDXLIfStmt(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a FuncExpr
			static
			CDXLScalar *MakeDXLFuncExpr(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a AggRef
			static
			CDXLScalar *MakeDXLAggFunc(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a scalar window function (WindowRef)
			static
			CDXLScalar *PdxlopWindowRef(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create an array
			static
			CDXLScalar *PdxlopArray(CDXLMemoryManager *memory_manager_dxl, const Attributes &attr);

			// create a proj elem
			static
			CDXLScalar *PdxlopProjElem(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a hash expr 
			static
			CDXLScalar *PdxlopHashExpr(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a sort col
			static
			CDXLScalar *PdxlopSortCol(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create an object representing cost estimates of a physical operator
			// from the parsed XML attributes
			static
			CDXLOperatorCost *GetOperatorCostDXL(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a table descriptor element
			static
			CDXLTableDescr *GetTableDescr(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create an index descriptor
			static
			CDXLIndexDescr *GetIndexDescr(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// create a column descriptor object
			static
			CDXLColDescr *GetColumnDescrAt(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);
			
			// create a column reference object
			static
			CDXLColRef *Pdxlcr(CDXLMemoryManager *memory_manager_dxl, const Attributes &, Edxltoken);
			
			// create a logical join 
			static
			CDXLLogical *PdxlopLogicalJoin(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// parse an output segment index
			static
			INT IOutputSegId(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// parse a grouping column id
			static
			ULONG UlGroupingColId(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs);

			// extracts the value for the given attribute.
			// if there is no such attribute defined, and the given optional
			// flag is set to false then it will raise an exception
			static
			const XMLCh *XmlstrFromAttrs
				(
				const Attributes &,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false
				);

			// extracts the boolean value for the given attribute
			// will raise an exception if value cannot be converted to a boolean
			static
			BOOL FValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// converts the XMLCh into LINT
			static
			LINT LValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// extracts the LINT value for the given attribute
			static
			LINT LValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				LINT lDefaultValue = 0
				);

			// converts the XMLCh into CDouble
			static
			CDouble DValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// cxtracts the double value for the given attribute
			static
			CDouble DValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// converts the XMLCh into ULONG. Will raise an exception if the 
			// argument cannot be converted to ULONG
			static
			ULONG UlValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// converts the XMLCh into ULLONG. Will raise an exception if the
			// argument cannot be converted to ULLONG
			static
			ULLONG UllValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// converts the XMLCh into INT. Will raise an exception if the 
			// argument cannot be converted to INT
			static
			INT IValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a INT value from the value for a given attribute
			// will raise an exception if the argument cannot be converted to INT
			static
			INT IValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				INT iDefaultValue = 0
				);

			// converts the XMLCh into short int. Will raise an exception if the
			// argument cannot be converted to short int
			static
			SINT SValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a short int value from the value for a given attribute
			// will raise an exception if the argument cannot be converted to short int
			static
			SINT SValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				SINT sDefaultValue = 0
				);

			// converts the XMLCh into char. Will raise an exception if the
			// argument cannot be converted to char
			static
			CHAR CValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// converts the XMLCh into oid. Will raise an exception if the
			// argument cannot be converted to OID
			static
			OID OidValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse an oid value from the value for a given attribute
			// will raise an exception if the argument cannot be converted to OID
			static
			OID OidValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				OID OidDefaultValue = 0
				);

			// parse a bool value from the value for a given attribute
			static
			BOOL FValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				BOOL fDefaultValue = false
				);
			
			// parse a string value from the value for a given attribute
			static
			CHAR *SzValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				CHAR *szDefaultValue = NULL
				);

			// parse a string value from the value for a given attribute
			static
			CHAR* SzValueFromXmlstr
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a string value from the value for a given attribute
			static
			CWStringDynamic *PstrValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a ULONG value from the value for a given attribute
			// will raise an exception if the argument cannot be converted to ULONG
			static
			ULONG UlValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				ULONG ulDefaultValue = 0
				);
			
			// parse a ULLONG value from the value for a given attribute
			// will raise an exception if the argument cannot be converted to ULLONG
			static
			ULLONG UllValueFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				ULLONG ullDefaultValue = 0
				);

			// parse an mdid object from the given attributes
			static
			IMDId *PmdidFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement,
				BOOL fOptional = false,
				IMDId *pmdidDefault = NULL
				);
			
			// parse an mdid object from an XMLCh
			static
			IMDId *PmdidFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlszMdid,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a GPDB mdid object from an array of its components
			static
			CMDIdGPDB *PmdidGPDB
				(
				CDXLMemoryManager *memory_manager_dxl,
				DrgPxmlsz *pdrgpxmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a GPDB CTAS mdid object from an array of its components
			static
			CMDIdGPDB *PmdidGPDBCTAS
				(
				CDXLMemoryManager *memory_manager_dxl,
				DrgPxmlsz *pdrgpxmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a column stats mdid object from an array of its components
			static
			CMDIdColStats *PmdidColStats
				(
				CDXLMemoryManager *memory_manager_dxl,
				DrgPxmlsz *pdrgpxmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a relation stats mdid object from an array of its components
			static
			CMDIdRelStats *PmdidRelStats
				(
				CDXLMemoryManager *memory_manager_dxl,
				DrgPxmlsz *pdrgpxmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a cast func mdid from the array of its components
			static
			CMDIdCast *PmdidCastFunc
				(
				CDXLMemoryManager *memory_manager_dxl,
				DrgPxmlsz *pdrgpxmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				); 
			
			// parse a comparison operator mdid from the array of its components
			static
			CMDIdScCmp *PmdidScCmp
				(
				CDXLMemoryManager *memory_manager_dxl,
				DrgPxmlsz *pdrgpxmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a dxl datum object
			static
			CDXLDatum *Pdxldatum
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attrs,
				Edxltoken edxltokenElement
				);
			
			// parse a comma-separated list of MDids into a dynamic array
			// will raise an exception if list is not well-formed
			static
			DrgPmdid *PdrgpmdidFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlszUlList,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a comma-separated list of unsigned long numbers into a dynamic array
			// will raise an exception if list is not well-formed
			static
			ULongPtrArray *PdrgpulFromAttrs
				(
				CDXLMemoryManager *memory_manager_dxl,
				const Attributes &attr,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a comma-separated list of integers numbers into a dynamic array
			// will raise an exception if list is not well-formed
			template <typename T, void (*CleanupFn)(T*),
					T ValueFromXmlstr(CDXLMemoryManager *, const XMLCh *, Edxltoken, Edxltoken)>
			static
			CDynamicPtrArray<T, CleanupFn> *PdrgptFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlszUl,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			static
			ULongPtrArray *PdrgpulFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlszUl,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				)
			{
				return PdrgptFromXMLCh<ULONG, CleanupDelete, UlValueFromXmlstr>
						(
						memory_manager_dxl,
						xmlszUl,
						edxltokenAttr,
						edxltokenElement
						);
			}

			static
			IntPtrArray *PdrgpiFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlszUl,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				)
			{
				return PdrgptFromXMLCh<INT, CleanupDelete, IValueFromXmlstr>
						(
						memory_manager_dxl,
						xmlszUl,
						edxltokenAttr,
						edxltokenElement
						);
			}

			// parse a comma-separated list of CHAR partition types into a dynamic array.
			// will raise an exception if list is not well-formed
			static
			CharPtrArray *PdrgpszFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);

			// parse a semicolon-separated list of comma-separated unsigned 
			// long numbers into a dynamc array of unsigned integer arrays
			// will raise an exception if list is not well-formed
			static
			ULongPtrArray2D *PdrgpdrgpulFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a comma-separated list of segment ids into a dynamic array
			// will raise an exception if list is not well-formed
			static
			IntPtrArray *PdrgpiParseSegmentIdList
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlszSegIdList,
				Edxltoken edxltokenAttr,
				Edxltoken edxltokenElement
				);
			
			// parse a comma-separated list of strings into a dynamic array
			// will raise an exception if list is not well-formed
			static
			StringPtrArray *PdrgPstrFromXMLCh
				(
				CDXLMemoryManager *memory_manager_dxl,
				const XMLCh *xmlsz
				);
			
			// parses the input and output segment ids from Xerces attributes and
			// stores them in the provided DXL Motion operator
			// will raise an exception if lists are not well-formed 
			static
			void SetSegmentInfo
				(
				CDXLMemoryManager *memory_pool,
				CDXLPhysicalMotion *pdxlopMotion,
				const Attributes &attrs,
				Edxltoken edxltokenElement
				);
			
			static
			EdxlJoinType EdxljtParseJoinType(const XMLCh *xmlszJoinType, const CWStringConst *pstrJoinName);
			
			static
			EdxlIndexScanDirection EdxljtParseIndexScanDirection
				(
				const XMLCh *xmlszIndexScanDirection,
				const CWStringConst *pstrIndexScanDirection
				);

			// parse system id
			static
			CSystemId Sysid(CDXLMemoryManager *memory_manager_dxl, const Attributes &attrs, Edxltoken edxltokenAttr, Edxltoken edxltokenElement);
			
			// parse the frame boundary
			static
			EdxlFrameBoundary Edxlfb(const Attributes& attrs, Edxltoken token_type);

			// parse the frame specification
			static
			EdxlFrameSpec Edxlfs(const Attributes& attrs);

			// parse the frame exclusion strategy
			static
			EdxlFrameExclusionStrategy Edxlfes(const Attributes& attrs);
			
			// parse comparison operator type
			static
			IMDType::ECmpType Ecmpt(const XMLCh* xmlszCmpType);
			
			// parse the distribution policy from the given XML string
			static
			IMDRelation::Ereldistrpolicy EreldistrpolicyFromXmlstr(const XMLCh *xmlsz);
			
			// parse the storage type from the given XML string
			static
			IMDRelation::Erelstoragetype ErelstoragetypeFromXmlstr(const XMLCh *xmlsz);
			
			// parse the OnCommit action spec for CTAS
			static
			CDXLCtasStorageOptions::ECtasOnCommitAction EctascommitFromAttr(const Attributes &attr);
			
			// parse index type
			static
			IMDIndex::EmdindexType EmdindtFromAttr(const Attributes &attrs);

	};

	// parse a comma-separated list of integers numbers into a dynamic array
	// will raise an exception if list is not well-formed
	template <typename T, void (*CleanupFn)(T*),
			T ValueFromXmlstr(CDXLMemoryManager *, const XMLCh *, Edxltoken, Edxltoken)>
	CDynamicPtrArray<T, CleanupFn> *
	CDXLOperatorFactory::PdrgptFromXMLCh
		(
		CDXLMemoryManager *memory_manager_dxl,
		const XMLCh *xmlszUlList,
		Edxltoken edxltokenAttr,
		Edxltoken edxltokenElement
		)
	{
		// get the memory pool from the memory manager
		IMemoryPool *memory_pool = memory_manager_dxl->Pmp();

		CDynamicPtrArray<T, CleanupFn> *pdrgpt = GPOS_NEW(memory_pool) CDynamicPtrArray<T, CleanupFn>(memory_pool);

		XMLStringTokenizer xmlsztok(xmlszUlList, CDXLTokens::XmlstrToken(EdxltokenComma));
		const ULONG ulNumTokens = xmlsztok.countTokens();

		for (ULONG ul = 0; ul < ulNumTokens; ul++)
		{
			XMLCh *xmlszNext = xmlsztok.nextToken();

			GPOS_ASSERT(NULL != xmlszNext);

			T *pt = GPOS_NEW(memory_pool) T(ValueFromXmlstr(memory_manager_dxl, xmlszNext, edxltokenAttr, edxltokenElement));
			pdrgpt->Append(pt);
		}

		return pdrgpt;
	}
}

#endif // !GPDXL_CDXLOperatorFactory_H

// EOF
