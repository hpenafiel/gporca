//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CTestUtils.h
//
//	@doc:
//		Optimizer test utility functions
//---------------------------------------------------------------------------
#ifndef GPOPT_CTestUtils_H
#define GPOPT_CTestUtils_H

#include "gpos/base.h"

#include "gpopt/base/CAutoOptCtxt.h"
#include "gpopt/base/CColRef.h"
#include "gpopt/base/CQueryContext.h"
#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarArrayCmp.h"
#include "gpopt/operators/CScalarConst.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSubquery.h"
#include "naucrates/md/CMDTypeInt2GPDB.h"
#include "naucrates/md/CMDTypeInt4GPDB.h"
#include "naucrates/md/CMDTypeInt8GPDB.h"
#include "naucrates/md/CSystemId.h"

#include "gpdbcost/CCostModelGPDBLegacy.h"

#include "unittest/base.h"

#include "naucrates/statistics/CPoint.h"


#define GPOPT_MDCACHE_TEST_OID	1234
#define GPOPT_MDCACHE_TEST_OID_PARTITIONED 5678
#define GPOPT_MDCACHE_TEST_OID_PARTITIONED_WITH_INDEXES 5369655
#define GPOPT_TEST_REL_WIDTH	3  // number of columns in generated test relations
#define GPOPT_TEST_PART_INDEX 1234567

#define GPOPT_TEST_REL_OID1	OID(11111)
#define GPOPT_TEST_REL_OID2	OID(22222)
#define GPOPT_TEST_REL_OID3	OID(33333)
#define GPOPT_TEST_REL_OID4	OID(44444)
#define GPOPT_TEST_REL_OID5	OID(55555)
#define GPOPT_TEST_REL_OID6	OID(66666)
#define GPOPT_TEST_REL_OID7	OID(77777)
#define GPOPT_TEST_REL_OID8	OID(88888)
#define GPOPT_TEST_REL_OID9	OID(99999)
#define GPOPT_TEST_REL_OID10 OID(10000)
#define GPOPT_TEST_REL_OID11 OID(10111)
#define GPOPT_TEST_REL_OID12 OID(10222)
#define GPOPT_TEST_REL_OID13 OID(10333)
#define GPOPT_TEST_REL_OID14 OID(10444)
#define GPOPT_TEST_REL_OID15 OID(10555)
#define GPOPT_TEST_REL_OID16 OID(10666)
#define GPOPT_TEST_REL_OID17 OID(10777)
#define GPOPT_TEST_REL_OID18 OID(10888)
#define GPOPT_TEST_REL_OID19 OID(10999)
#define GPOPT_TEST_REL_OID20 OID(10200)
#define GPOPT_TEST_REL_OID21 OID(10311)
#define GPOPT_TEST_REL_OID22 OID(27118)

#define GPDB_INT4_LT_OP OID(97)
#define GPDB_INT4_EQ_OP OID(96)
#define GPDB_INT4_NEQ_OP OID(518)
#define GPDB_INT4_SUM_AGG OID(2114)
#define GPDB_INT8_GENERATE_SERIES OID(1067)

// forward declarations
namespace gpmd
{
	class CMDIdGPDB;
}

namespace gpopt
{
	using namespace gpos;
	using gpmd::CMDIdGPDB;

	// forward declarations
	class CConstraintInterval;
	class IConstExprEvaluator;

	typedef CDynamicPtrArray<CExpression, CleanupNULL> DrgPexprJoins;

	typedef BOOL (FnDXLPlanChecker)(CDXLNode *);

	//---------------------------------------------------------------------------
	//	@class:
	//		CTestUtils
	//
	//	@doc:
	//		Test utility functions
	//
	//---------------------------------------------------------------------------
	class CTestUtils
	{

		private:

			// local memory pool
			static
			IMemoryPool *m_memory_pool;

			// construct an array of segment ids
			static gpdxl::IntPtrArray *PdrgpiSegments(IMemoryPool *memory_pool);

			// generate minidump file name from passed file name
			static
			CHAR *SzMinidumpFileName(IMemoryPool *memory_pool, const CHAR *szFileName);

		public:

			// pair of DXL query file and the corresponding DXL plan file
			struct STestCase
			{
				const CHAR *szInputFile;
				const CHAR *szOutputFile;
			};

			//---------------------------------------------------------------------------
			//	@class:
			//		CTestUtils
			//
			//	@doc:
			//		Set up basic structures needed for most tests
			//
			//---------------------------------------------------------------------------
			class CTestSetup
			{
				private:
					// memory pool
					CAutoMemoryPool m_amp;

					// metadata accessor
					CMDAccessor m_mda;

					// optimization context
					CAutoOptCtxt m_aoc;

					// private ctor
					CTestSetup(const CTestSetup &);

					// set up a file based provider
					static CMDProviderMemory *PmdpSetupFileBasedProvider();
				public:
					// ctor
					CTestSetup();

					// return the memory pool
					IMemoryPool *Pmp()
					{
						return m_amp.Pmp();
					}

					// return the metadata acccessor
					CMDAccessor *Pmda()
					{
						return &m_mda;
					}
			};


			//-------------------------------------------------------------------
			// Initialization/Destruction of provider file
			//-------------------------------------------------------------------
			// initialize provider file
			static
			void InitProviderFile(IMemoryPool *memory_pool);

			// destroy metadata provider
			static
			void DestroyMDProvider();

			//-------------------------------------------------------------------
			// Helpers for generating expressions
			//-------------------------------------------------------------------

			// generate a plain table descriptor
			static
			CTableDescriptor *PtabdescPlain(IMemoryPool *memory_pool, ULONG ulCols, IMDId *mdid, const CName &nameTable, BOOL fNullable = false);

			// generate a plain table descriptor, where the column names are generated using a format string containing %d
			static
			CTableDescriptor *PtabdescPlainWithColNameFormat(IMemoryPool *memory_pool, ULONG ulCols, IMDId *mdid, const WCHAR *wszColNameFormat, const CName &nameTable, BOOL fNullable = false);

			// generate a table descriptor
			static
			CTableDescriptor *PtabdescCreate(IMemoryPool *memory_pool, ULONG ulCols, IMDId *mdid, const CName &name, BOOL fPartitioned = false);

			// generate a get expression
			static
			CExpression *PexprLogicalGet(IMemoryPool *memory_pool, CTableDescriptor *ptabdesc, const CWStringConst *pstrTableAlias);

			// generate a get expression over table with nullable columns
			static
			CExpression *PexprLogicalGetNullable(IMemoryPool *memory_pool, OID oidTable, const CWStringConst *str_table_name, const CWStringConst *pstrTableAlias);

			// generate a get expression
			static
			CExpression *PexprLogicalGet(IMemoryPool *memory_pool, CWStringConst *str_table_name, CWStringConst *pstrTableAlias, ULONG ulTableId);

			// generate a random get expression
			static
			CExpression *PexprLogicalGet(IMemoryPool *memory_pool);

			// generate a random external get expression
			static
			CExpression *PexprLogicalExternalGet(IMemoryPool *memory_pool);

			// generate a random assert expression
			static
			CExpression *PexprLogicalAssert(IMemoryPool *memory_pool);
			
			// generate a random get expression on a partitioned table
			static
			CExpression *PexprLogicalGetPartitioned(IMemoryPool *memory_pool);

			// generate a randomized get expression for a partitioned table with indexes
			static
			CExpression *PexprLogicalDynamicGetWithIndexes(IMemoryPool *memory_pool);

			// generate a select expression with random equality predicate
			static
			CExpression *PexprLogicalSelect(IMemoryPool *memory_pool, CExpression *pexpr);

			// generate a select expression
			static
			CExpression *PexprLogicalSelect(IMemoryPool *memory_pool, CWStringConst *str_table_name, CWStringConst *pstrTableAlias, ULONG ulTableId);

			// generate a random select expression
			static
			CExpression *PexprLogicalSelect(IMemoryPool *memory_pool);

			// generate a select expression with a contradiction
			static
			CExpression *PexprLogicalSelectWithContradiction(IMemoryPool *memory_pool);

			// generate a select on top of a partitioned table get with a filter on the part key
			static
			CExpression *PexprLogicalSelectPartitioned(IMemoryPool *memory_pool);

			// generate a project element with sum agg function
			static
			CExpression *PexprPrjElemWithSum(IMemoryPool *memory_pool, CColRef *pcr);

			// generate an join of specific join type on  get with a predicate 
			// on the part key with a partitioned table on the outer side 
			template<class T>
			static
			CExpression *PexprJoinPartitionedOuter(IMemoryPool *memory_pool);
			
			// generate a join of specific join type on  get with a predicate 
			// on the part key with a partitioned table on the inner side 
			template<class T>
			static
			CExpression *PexprJoinPartitionedInner(IMemoryPool *memory_pool);
			
			// generate an join of specific join type on top of a partitioned 
			// table get with a predicate on the part key
			template<class T>
			static
			CExpression *PexprJoinPartitioned(IMemoryPool *memory_pool, BOOL fOuterPartitioned);
			
			// generate an 3-way join including a partitioned table
			static
			CExpression *Pexpr3WayJoinPartitioned(IMemoryPool *memory_pool);

			// generate an 4-way join including a partitioned table
			static
			CExpression *Pexpr4WayJoinPartitioned(IMemoryPool *memory_pool);

			// generate a random select expression with nested AND tree
			static
			CExpression *PexprLogicalSelectWithNestedAnd(IMemoryPool *memory_pool);

			// generate a random select expression with nested OR tree
			static
			CExpression *PexprLogicalSelectWithNestedOr(IMemoryPool *memory_pool);

			// generate a random select expression with an even number of nested NOT nodes
			static
			CExpression *PexprLogicalSelectWithEvenNestedNot(IMemoryPool *memory_pool);

			// generate a random select expression with an odd number of nested NOT nodes
			static
			CExpression *PexprLogicalSelectWithOddNestedNot(IMemoryPool *memory_pool);

			// generate a random select expression with nested AND-OR tree
			static
			CExpression *PexprLogicalSelectWithNestedAndOrNot(IMemoryPool *memory_pool);

			// generate a Select expression with ANY/ALL subquery predicate over a const table get
			static
			CExpression *PexprLogicalSubqueryWithConstTableGet(IMemoryPool *memory_pool, COperator::EOperatorId eopid);

			// generate a random select expression with constant ANY subquery
			static
			CExpression *PexprLogicalSelectWithConstAnySubquery(IMemoryPool *memory_pool);

			// generate a random select expression with constant ALL subquery
			static
			CExpression *PexprLogicalSelectWithConstAllSubquery(IMemoryPool *memory_pool);

			// generate a correlated select expression
			static
			CExpression *PexprLogicalSelectCorrelated(IMemoryPool *memory_pool);

			// generate a correlated select expression
			static
			CExpression *PexprLogicalSelectCorrelated(IMemoryPool *memory_pool, CColRefSet *pcrsOuter, ULONG ulLevel);

			// generate a select on top of outer join expression
			static
			CExpression *PexprLogicalSelectOnOuterJoin(IMemoryPool *memory_pool);

			// generate a join expression with a random equality predicate
			template<class T>
			static
			CExpression *PexprLogicalJoin(IMemoryPool *memory_pool, CExpression *pexprLeft, CExpression *pexprRight);

			// generate a random binary join expression of specific join type
			template<class T>
			static
			CExpression *PexprLogicalJoin(IMemoryPool *memory_pool);

			// generate join with correlation beneath it
			static
			CExpression *PexprLogicalJoinCorrelated(IMemoryPool *memory_pool);
	
			// generate join with a partitioned and indexed inner child
			static
			CExpression *PexprLogicalJoinWithPartitionedAndIndexedInnerChild(IMemoryPool *memory_pool);
			
			// generate a select expression with predicate between a scalar ident and a constant
			static
			CExpression *PexprLogicalSelectCmpToConst(IMemoryPool *memory_pool);

			// generate a select expression with an array compare
			static
			CExpression *PexprLogicalSelectArrayCmp(IMemoryPool *memory_pool);

			// generate a select expression with an array compare
			static
			CExpression *PexprLogicalSelectArrayCmp(IMemoryPool *memory_pool, CScalarArrayCmp::EArrCmpType earrcmptype, IMDType::ECmpType ecmptype);

			// generate a select expression with an array compare
			static
			CExpression *PexprLogicalSelectArrayCmp(IMemoryPool *memory_pool, CScalarArrayCmp::EArrCmpType earrcmptype, IMDType::ECmpType ecmptype, const IntPtrArray *pdrgpiVals);
			
			// generate an n-ary join expression
			static
			CExpression *PexprLogicalNAryJoin(IMemoryPool *memory_pool, DrgPexpr *pdrgpexpr);

			// generate an n-ary join expression using given array of relation names
			static
			CExpression *PexprLogicalNAryJoin(IMemoryPool *memory_pool, CWStringConst *pstrRel, ULONG *pulRel, ULONG ulRels, BOOL fCrossProduct);

			// generate a random n-ary expression
			static
			CExpression *PexprLogicalNAryJoin(IMemoryPool *memory_pool);

			// generate left outer join on top of n-ary join expression
			static
			CExpression *PexprLeftOuterJoinOnNAryJoin(IMemoryPool *memory_pool);

			// generate n-ary join on top of left outer join expression
			static
			CExpression *PexprNAryJoinOnLeftOuterJoin(IMemoryPool *memory_pool);

			// generate a project expression
			static
			CExpression *PexprLogicalProject(IMemoryPool *memory_pool, CExpression *pexpr, CColRef *pcr, CColRef *pcrNew);

			// generate a random project expression
			static
			CExpression *PexprLogicalProject(IMemoryPool *memory_pool);

			// generate a Project over GbAgg with correlation beneath it
			static
			CExpression *PexprLogicalProjectGbAggCorrelated(IMemoryPool *memory_pool);

			// generate a limit expression
			static
			CExpression *PexprLogicalLimit(IMemoryPool *memory_pool, CExpression *pexpr, LINT iStart, LINT iRows, BOOL fGlobal, BOOL fHasCount);

			// generate a random limit expression
			static
			CExpression *PexprLogicalLimit(IMemoryPool *memory_pool);

			// generate a random group by expression
			static
			CExpression *PexprLogicalGbAgg(IMemoryPool *memory_pool);

			// generate a random group by expression
			static
			CExpression *PexprLogicalGbAggWithInput(IMemoryPool *memory_pool, CExpression *pexprInput);

			// generate a random group by with a sum expression
			static
			CExpression *PexprLogicalGbAggWithSum(IMemoryPool *memory_pool);

			// generate a random group by over join expression
			static
			CExpression *PexprLogicalGbAggOverJoin(IMemoryPool *memory_pool);

			// generate GbAgg with correlation beneath it
			static
			CExpression *PexprLogicalGbAggCorrelated(IMemoryPool *memory_pool);

			// generate a dedup group by over inner join expression
			static
			CExpression *PexprLogicalGbAggDedupOverInnerJoin(IMemoryPool *memory_pool);

			// generate a top-level apply expression
			template<class T>
			static
			CExpression *PexprLogicalApply(IMemoryPool *memory_pool);

			// generate a top-level apply expression with an outer reference
			template<class T>
			static
			CExpression *PexprLogicalApplyWithOuterRef(IMemoryPool *memory_pool);

			// generate a const table get expression
			static
			CExpression *PexprConstTableGet(IMemoryPool *memory_pool, ULONG ulElements);
			
			// generate a const table get expression with 5 elements
			static
			CExpression *PexprConstTableGet5(IMemoryPool *memory_pool);

			// generate a logical insert expression
			static
			CExpression *PexprLogicalInsert(IMemoryPool *memory_pool);

			// generate a logical delete expression
			static
			CExpression *PexprLogicalDelete(IMemoryPool *memory_pool);

			// generate a logical update expression
			static
			CExpression *PexprLogicalUpdate(IMemoryPool *memory_pool);

			// generate a dynamic get expression
			static
			CExpression *PexprLogicalDynamicGet
							(
							IMemoryPool *memory_pool,
							CTableDescriptor *ptabdesc,
							const CWStringConst *pstrTableAlias,
							ULONG ulPartIndex
							);
			
			// generate a dynamic get expression
			static
			CExpression *PexprLogicalDynamicGet(IMemoryPool *memory_pool);

			// generate a select with an equality predicate over a dynamic get expression
			static
			CExpression *PexprLogicalSelectWithEqPredicateOverDynamicGet(IMemoryPool *memory_pool);
			
			// generate a select with a less than predicate over a dynamic get expression
			static
			CExpression *PexprLogicalSelectWithLTPredicateOverDynamicGet(IMemoryPool *memory_pool);
			
			// generate a logical table-valued function expression with 2 constant arguments
			static
			CExpression *PexprLogicalTVFTwoArgs(IMemoryPool *memory_pool);

			// generate a logical table-valued function expression with 3 constant arguments
			static
			CExpression *PexprLogicalTVFThreeArgs(IMemoryPool *memory_pool);

			// generate a logical table-valued function expression with no arguments
			static
			CExpression *PexprLogicalTVFNoArgs(IMemoryPool *memory_pool);

			// generate a logical table-valued function expression
			static
			CExpression *PexprLogicalTVF(IMemoryPool *memory_pool, ULONG ulArgs);

			// generate a cte producer on top of a select
			static
			CExpression *PexprLogicalCTEProducerOverSelect(IMemoryPool *memory_pool, ULONG ulCTEId);

			// generate a CTE producer on top of a select
			static
			CExpression *PexprLogicalCTEProducerOverSelect(IMemoryPool *memory_pool);

			// generate an expression with CTE producer and consumer
			static
			CExpression *PexprCTETree(IMemoryPool *memory_pool);

			// generate a logical sequence expression
			static
			CExpression *PexprLogicalSequence(IMemoryPool *memory_pool, DrgPexpr *pdrgpexpr); 

			// generate a logical sequence expression
			static
			CExpression *PexprLogicalSequence(IMemoryPool *memory_pool);

			// generate a random union expression
			static
			CExpression *PexprLogicalUnion(IMemoryPool *memory_pool, ULONG ulDepth);

			// generate a sequence project expression
			static
			CExpression *PexprLogicalSequenceProject(IMemoryPool *memory_pool, OID oidFunc, CExpression *pexprInput);

			// generate a random expression with one window function
			static
			CExpression *PexprOneWindowFunction(IMemoryPool *memory_pool);

			// generate a random expression with two window functions
			static
			CExpression *PexprTwoWindowFunctions(IMemoryPool *memory_pool);

			// generate a dynamic array of join expressions
			static
			DrgPexprJoins *PdrgpexprJoins(IMemoryPool *memory_pool, CWStringConst *pstrRel, ULONG *pulRel, ULONG ulRels, BOOL fCrossProduct);
							
			// generate a query context from an array of required column references
			static
			CQueryContext *PqcGenerate(IMemoryPool *memory_pool, CExpression *pexpr, DrgPcr *pdrgpcr);

			// generate a dummy query context for testing
			static
			CQueryContext *PqcGenerate(IMemoryPool *memory_pool, CExpression *pexpr);

			// generate a nested AND/OR tree
			static
			CExpression *PexprScalarNestedPreds(IMemoryPool *memory_pool, CExpression *pexpr, CScalarBoolOp::EBoolOperator eboolop);

			// find the first child expression with the given operator id
			static
			CExpression *PexprFindFirstExpressionWithOpId(CExpression *pexpr, COperator::EOperatorId eopid);

			// equality predicate shortcut
			static
			void EqualityPredicate(IMemoryPool *memory_pool, CColRefSet *pcrsLeft, CColRefSet *pcrsRight, DrgPexpr *pdrgpexpr);

			// int2 point
			static
			CPoint *PpointInt2(IMemoryPool *memory_pool, INT i);

			// int4 point
			static
			CPoint *PpointInt4(IMemoryPool *memory_pool, INT i);

			// create an INT4 point with null value
			static
			CPoint *PpointInt4NullVal(IMemoryPool *memory_pool);

			// int8 point
			static
			CPoint *PpointInt8(IMemoryPool *memory_pool, INT li);

			// bool point
			static
			CPoint *PpointBool(IMemoryPool *memory_pool, BOOL value);

			// perform DXL(query) -> CExpression(Log) ->CExpression(Ph) -> DXL(plan)
			// and check whether the resulting plan DXL matches the expected output
			static
			GPOS_RESULT EresTranslate
								(
								IMemoryPool *memory_pool,
								const CHAR *szQueryFileName,
								const CHAR *szPlanFileName,
								BOOL fIgnoreMismatch
								);

			// return query logical expression from minidump
			static
			CExpression *PexprReadQuery
				(
				IMemoryPool *memory_pool,
				const CHAR *szQueryFileName
				);

			// compare expected and actual output
			static
			GPOS_RESULT EresCompare
								(
								IOstream &os,
								CWStringDynamic *pstrActual,
								CWStringDynamic *pstrExpected,
								BOOL fIgnoreMismatch = true
								);
			
			// match given two plans using string comparison
			static
			BOOL FPlanMatch
				(
				IMemoryPool *memory_pool,
				IOstream &os,
				const CDXLNode *first_child_dxlnode,
				ULLONG ullPlanIdFst,
				ULLONG ullPlanSpaceSizeFst,
				const CDXLNode *second_child_dxlnode,
				ULLONG ullPlanIdSnd,
				ULLONG ullPlanSpaceSizeSnd
				);

			// compare given DXL plans
			static
			BOOL FPlanCompare
				(
				IMemoryPool *memory_pool,
				IOstream &os,
				const CDXLNode *first_child_dxlnode,
				ULLONG ullPlanIdFst,
				ULLONG ullPlanSpaceSizeFst,
				const CDXLNode *second_child_dxlnode,
				ULLONG ullPlanIdSnd,
				ULLONG ullPlanSpaceSizeSnd,
				BOOL fMatchPlans,
				INT iCmpSpaceSize
				);

			// run minidumps while loading metadata from passed file
			static
			GPOS_RESULT EresRunMinidumps
				(
				IMemoryPool *memory_pool,
				const CHAR *rgszFileNames[],
				ULONG ulTests,
				ULONG *pulTestCounter,
				ULONG ulSessionId,
				ULONG ulCmdId,
				BOOL fMatchPlans,
				BOOL fTestSpacePruning,
				const CHAR *szMDFilePath =  NULL,
				IConstExprEvaluator *pceeval = NULL
				);

			// run all minidumps based on one metadata file
			static
			GPOS_RESULT EresRunMinidumpsUsingOneMDFile
				(
				IMemoryPool *memory_pool,
				const CHAR *szMDFilePath,
				const CHAR *rgszFileNames[],
				ULONG *pulTestCounter,
				ULONG ulSessionId,
				ULONG ulCmdId,
				BOOL fMatchPlans,
				INT iCmpSpaceSize,
				IConstExprEvaluator *pceeval = NULL
				);

			// run one minidump-based test using passed MD Accessor
			static
			GPOS_RESULT EresRunMinidump
				(
				IMemoryPool *memory_pool,
				CMDAccessor *md_accessor,
				const CHAR *rgszFileName,
				ULONG *pulTestCounter,
				ULONG ulSessionId,
				ULONG ulCmdId,
				BOOL fMatchPlans,
				INT iCmpSpaceSize,
				IConstExprEvaluator *pceeval
				);

			// test plan sampling
			static
			GPOS_RESULT EresSamplePlans
				(
				const CHAR *rgszFileNames[],
				ULONG ulTests,
				ULONG *pulTestCounter,
				ULONG ulSessionId,
				ULONG ulCmdId
				);
			
			// check all enumerated plans using given PlanChecker function
			static
			GPOS_RESULT EresCheckPlans
				(
				const CHAR *rgszFileNames[],
				ULONG ulTests,
				ULONG *pulTestCounter,
				ULONG ulSessionId,
				ULONG ulCmdId,
				FnPlanChecker *pfpc
				);

			// check the optimized plan using given DXLPlanChecker function
			// cost model must be calibrated
			// does not take ownership of pdrgpcp
			static
			GPOS_RESULT EresCheckOptimizedPlan
				(
				const CHAR *rgszFileNames[],
				ULONG ulTests,
				ULONG *pulTestCounter,
				ULONG ulSessionId,
				ULONG ulCmdId,
				FnDXLPlanChecker *pfdpc,
				DrgPcp *pdrgpcp
				);

			// generate cost model used in tests
			static
			ICostModel *GetCostModel
				(
				IMemoryPool *memory_pool
				)
			{
				return GPOS_NEW(memory_pool) CCostModelGPDBLegacy(memory_pool, GPOPT_TEST_SEGMENTS);
			}

			// create a datum with a given type, encoded value and int value
			static
			IDatum *PdatumGeneric(IMemoryPool *memory_pool, CMDAccessor *md_accessor, IMDId *mdid_type, CWStringDynamic *pstrEncodedValue, LINT value);

			// create an interval for generic data types
			// does not take ownership of mdid_type
			static
			CConstraintInterval *PciGenericInterval
			        (
			        IMemoryPool *memory_pool,
			        CMDAccessor *md_accessor,
			        const CMDIdGPDB &mdidType,
			        CColRef *pcr,
			        CWStringDynamic *pstrLower,
			        LINT lLower,
			        CRange::ERangeInclusion eriLeft,
			        CWStringDynamic *pstrUpper,
			        LINT lUpper,
			        CRange::ERangeInclusion eriRight
			        );

			// helper for generating a scalar compare ident to constant
			static
			CExpression *PexprScalarCmpIdentToConstant(IMemoryPool *memory_pool, CExpression *pexpr);

			// helper for generating an exists subquery
			static
			CExpression *PexprExistsSubquery(IMemoryPool *memory_pool, CExpression *pexprOuter);

			// helper for generating a not exists subquery
			static
			CExpression *PexprNotExistsSubquery(IMemoryPool *memory_pool, CExpression *pexprOuter);

			// helper for generating an ANY subquery
			static
			CExpression *PexpSubqueryAny(IMemoryPool *memory_pool, CExpression *pexprOuter);

			// helper for generating an ALL subquery
			static
			CExpression *PexpSubqueryAll(IMemoryPool *memory_pool, CExpression *pexprOuter);

			// recursively traverses the subtree rooted at the given expression, and return
			// the first subexpression it encounters that has the given id
			static
			const CExpression *PexprFirst(const CExpression *pexpr, const COperator::EOperatorId eopid);

			// generate a scalar expression comparing scalar identifier to a constant
			static
			CExpression *PexprScIdentCmpConst(IMemoryPool *memory_pool, CExpression *pexprRelational, IMDType::ECmpType cmp_type, ULONG ulVal);

			// generate a scalar expression comparing scalar identifiers
			static
			CExpression *PexprScIdentCmpScIdent(IMemoryPool *memory_pool, CExpression *pexprLeft, CExpression *pexprRight, IMDType::ECmpType cmp_type);

			// generate a scalar AND expression
			static
			CExpression *PexprAnd(IMemoryPool *memory_pool, CExpression *pexprFst, CExpression *pexprSnd);

			// generate a scalar OR expression
			static
			CExpression *PexprOr(IMemoryPool *memory_pool, CExpression *pexprFst, CExpression *pexprSnd);

#ifdef GPOS_DEBUG
			static
			BOOL FFaultSimulation();
#endif // GPOS_DEBUG
			
			// default file-based metadata provider
			static
			CMDProviderMemory *m_pmdpf;

			// default source system id
			static
			CSystemId m_sysidDefault;

			// metadata file
			static
			const CHAR *m_szMDFileName;
			
			// xsd file
			static
			const CHAR *m_szXSDPath;

			// run Minidump-based tests in the given array of files
			static
			GPOS_RESULT EresUnittest_RunTests(const CHAR **rgszFileNames, ULONG *pulTestCounter, ULONG ulTests);

			static
			GPOS_RESULT EresUnittest_RunTestsWithoutAdditionalTraceFlags(const CHAR **rgszFileNames, ULONG *pulTestCounter, ULONG ulTests, BOOL fMatchPlans, BOOL fTestSpacePruning);

			// return the number of segments, default return GPOPT_TEST_SEGMENTS
			static
			ULONG UlSegments(COptimizerConfig *optimizer_config);

			// create Equivalence Class based on the breakpoints
			static
			DrgPcrs *
			createEquivalenceClasses(IMemoryPool *memory_pool, CColRefSet *pcrs, INT setBoundary[]);
	}; // class CTestUtils


	//---------------------------------------------------------------------------
	//		CTestUtils::PexprJoinPartitionedOuter
	//
	//	@doc:
	//		Generate a randomized join expression over a partitioned table
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprJoinPartitionedOuter
	(
	 IMemoryPool *memory_pool
	 )
	{
		return PexprJoinPartitioned<T>(memory_pool, true /*fOuterPartitioned*/);
	}

	//---------------------------------------------------------------------------
	//		CTestUtils::PexprJoinPartitionedInner
	//
	//	@doc:
	//		Generate a randomized join expression over a partitioned table
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprJoinPartitionedInner
	(
	 IMemoryPool *memory_pool
	 )
	{
		return PexprJoinPartitioned<T>(memory_pool, false /*fOuterPartitioned*/);
	}

	//---------------------------------------------------------------------------
	//		CTestUtils::PexprJoinPartitioned
	//
	//	@doc:
	//		Generate a randomized join expression over a partitioned table
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprJoinPartitioned
	(
	 IMemoryPool *memory_pool,
	 BOOL fOuterPartitioned
	 )
	{
		// create an outer Get expression and extract a random column from it
		CExpression *pexprGet = PexprLogicalGet(memory_pool);
		CColRefSet *pcrs = CDrvdPropRelational::Pdprel(pexprGet->PdpDerive())->PcrsOutput();
		CColRef *pcr =  pcrs->PcrAny();

		// create an inner partitioned Get expression
		CExpression *pexprGetPartitioned = PexprLogicalGetPartitioned(memory_pool);

		// extract first partition key
		CLogicalGet *popGetPartitioned = CLogicalGet::PopConvert(pexprGetPartitioned->Pop());
		const DrgDrgPcr *pdrgpdrgpcr = popGetPartitioned->PdrgpdrgpcrPartColumns();

		GPOS_ASSERT(pdrgpdrgpcr != NULL);
		GPOS_ASSERT(0 < pdrgpdrgpcr->Size());
		DrgPcr *pdrgpcr = (*pdrgpdrgpcr)[0];
		GPOS_ASSERT(1 == pdrgpcr->Size());
		CColRef *pcrPartKey = (*pdrgpcr)[0];

		// construct a comparison pk = a
		CExpression *pexprScalar = CUtils::PexprScalarEqCmp
		(
		 memory_pool,
		 CUtils::PexprScalarIdent(memory_pool, pcr),
		 CUtils::PexprScalarIdent(memory_pool, pcrPartKey)
		 );

		if (fOuterPartitioned)
		{
			return CUtils::PexprLogicalJoin<T>(memory_pool, pexprGetPartitioned, pexprGet, pexprScalar);
		}

		return CUtils::PexprLogicalJoin<T>(memory_pool, pexprGet, pexprGetPartitioned, pexprScalar);
	}

	//---------------------------------------------------------------------------
	//	@function:
	//		CTestUtils::PexprLogicalJoin
	//
	//	@doc:
	//		Generate a join expression with a random equality predicate
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprLogicalJoin
	(
	 IMemoryPool *memory_pool,
	 CExpression *pexprLeft,
	 CExpression *pexprRight
	 )
	{
		GPOS_ASSERT(NULL != pexprLeft);
		GPOS_ASSERT(NULL != pexprRight);

		// get any two columns; one from each side
		CDrvdPropRelational *pdprelLeft = CDrvdPropRelational::Pdprel(pexprLeft->PdpDerive());
		CColRef *pcrLeft = pdprelLeft->PcrsOutput()->PcrAny();

		CDrvdPropRelational *pdprelRight = CDrvdPropRelational::Pdprel(pexprRight->PdpDerive());
		CColRef *pcrRight = pdprelRight->PcrsOutput()->PcrAny();
		CExpression *pexprEquality = CUtils::PexprScalarEqCmp(memory_pool, pcrLeft, pcrRight);

		return CUtils::PexprLogicalJoin<T>(memory_pool, pexprLeft, pexprRight, pexprEquality);
	}

	//---------------------------------------------------------------------------
	//	@function:
	//		CTestUtils::PexprLogicalJoin
	//
	//	@doc:
	//		Generate randomized basic join expression
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprLogicalJoin
	(
	 IMemoryPool *memory_pool
	 )
	{
		CExpression *pexprLeft = PexprLogicalGet(memory_pool);
		CExpression *pexprRight = PexprLogicalGet(memory_pool);
		// reversing the order here is a workaround for non-determinism
		// in CExpressionPreprocessorTest where join order was flipped
		// depending on the compiler used
		return PexprLogicalJoin<T>(memory_pool, pexprRight, pexprLeft);
	}

	//---------------------------------------------------------------------------
	//	@function:
	//		CTestUtils::PexprLogicalApply
	//
	//	@doc:
	//		Generate top-level apply
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprLogicalApply
	(
	 IMemoryPool *memory_pool
	 )
	{
		CExpression *pexprOuter = PexprLogicalGet(memory_pool);
		CColRefSet *pcrsOuter = CDrvdPropRelational::Pdprel(pexprOuter->PdpDerive())->PcrsOutput();

		CExpression *pexprInner = PexprLogicalSelectCorrelated(memory_pool, pcrsOuter, 3);
		CExpression *pexprPredicate = CUtils::PexprScalarConstBool(memory_pool, true /*value*/);

		COperator *pop = GPOS_NEW(memory_pool) T(memory_pool);
		return GPOS_NEW(memory_pool) CExpression(memory_pool, pop, pexprOuter, pexprInner, pexprPredicate);
	}

	//---------------------------------------------------------------------------
	//	@function:
	//		CTestUtils::PexprLogicalApplyWithOuterRef
	//
	//	@doc:
	//		Generate top-level apply
	//
	//---------------------------------------------------------------------------
	template<class T>
	CExpression *
	CTestUtils::PexprLogicalApplyWithOuterRef
	(
	 IMemoryPool *memory_pool
	 )
	{
		CExpression *pexprOuter = PexprLogicalGet(memory_pool);
		CColRefSet *pcrsOuter = CDrvdPropRelational::Pdprel(pexprOuter->PdpDerive())->PcrsOutput();

		CExpression *pexprInner = PexprLogicalSelectCorrelated(memory_pool, pcrsOuter, 3);
		CExpression *pexprPredicate = CUtils::PexprScalarConstBool(memory_pool, true /*value*/);

		CColRefSet *pcrsOuterRef = CDrvdPropRelational::Pdprel(pexprInner->PdpDerive())->PcrsOuter();
		GPOS_ASSERT(1 == pcrsOuterRef->Size());
		CColRef *pcr = pcrsOuterRef->PcrFirst();

		DrgPcr *pdrgpcr = GPOS_NEW(memory_pool) DrgPcr(memory_pool);
		pdrgpcr->Append(pcr);
		COperator *pop = GPOS_NEW(memory_pool) T(memory_pool, pdrgpcr, COperator::EopScalarSubquery);
		return GPOS_NEW(memory_pool) CExpression(memory_pool, pop, pexprOuter, pexprInner, pexprPredicate);
	}

} // namespace gpopt


#endif // !GPOPT_CTestUtils_H

// EOF
