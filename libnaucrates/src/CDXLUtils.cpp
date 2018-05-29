//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2010 Greenplum, Inc.
//
//	@filename:
//		CDXLUtils.cpp
//
//	@doc:
//		Implementation of the utility methods for parsing and searializing DXL.
//---------------------------------------------------------------------------

#include "gpos/base.h"
#include "gpos/common/CAutoTimer.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/CAutoRg.h"

#include "gpos/io/ioutils.h"
#include "gpos/io/CFileReader.h"
#include "gpos/io/COstreamString.h"
#include "gpos/task/CAutoTraceFlag.h"
#include "gpos/task/CWorker.h"
#include "gpos/task/CTraceFlagIter.h"

#include "gpopt/engine/CEnumeratorConfig.h"
#include "gpopt/engine/CStatisticsConfig.h"
#include "gpopt/optimizer/COptimizerConfig.h"

#include "naucrates/base/CQueryToDXLResult.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/dxl/parser/CParseHandlerDXL.h"
#include "naucrates/dxl/parser/CParseHandlerPlan.h"

#include "naucrates/dxl/parser/CParseHandlerFactory.h"
#include "naucrates/dxl/parser/CParseHandlerManager.h"
#include "naucrates/dxl/parser/CParseHandlerDummy.h"
#include "naucrates/dxl/xml/CDXLMemoryManager.h"
#include "naucrates/dxl/xml/CXMLSerializer.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/base/COptCtxt.h"

#include "naucrates/md/CMDRequest.h"
#include "naucrates/md/CDXLStatsDerivedRelation.h"

#include "naucrates/traceflags/traceflags.h"

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/Base64.hpp>

using namespace gpdxl;
using namespace gpmd;
using namespace gpos;
using namespace gpopt;

XERCES_CPP_NAMESPACE_USE



//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::GetParseHandlerForDXLString
//
//	@doc:
//		Start the parsing of the given DXL string and return the top-level parser.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
CParseHandlerDXL *
CDXLUtils::GetParseHandlerForDXLString
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	// we need to disable OOM simulation here, otherwise xerces throws ABORT signal
	CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
	CAutoTraceFlag atf2(EtraceSimulateAbort, false);

	// setup own memory manager
	CDXLMemoryManager *memory_manager = GPOS_NEW(memory_pool) CDXLMemoryManager(memory_pool);
	SAX2XMLReader* sax_2_xml_reader = XMLReaderFactory::createXMLReader(memory_manager);

#ifdef GPOS_DEBUG
	CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG

	XMLCh *xsd_path = NULL;

	if (NULL != xsd_file_path)
	{
		// setup XSD validation
		sax_2_xml_reader->setFeature(XMLUni::fgSAX2CoreValidation, true);
		sax_2_xml_reader->setFeature(XMLUni::fgXercesDynamic, false);
		sax_2_xml_reader->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
		sax_2_xml_reader->setFeature(XMLUni::fgXercesSchema, true);
		
		sax_2_xml_reader->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
		sax_2_xml_reader->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);
		
		xsd_path = XMLString::transcode(xsd_file_path, memory_manager);
		sax_2_xml_reader->setProperty(XMLUni::fgXercesSchemaExternalSchemaLocation, (void*) xsd_path);
	}
			
	CParseHandlerManager *parse_handler_mgr = GPOS_NEW(memory_pool) CParseHandlerManager(memory_manager, sax_2_xml_reader);

	CParseHandlerDXL *parse_handler_dxl = CParseHandlerFactory::Pphdxl(memory_pool, parse_handler_mgr);

	parse_handler_mgr->ActivateParseHandler(parse_handler_dxl);
		
	MemBufInputSource *input_src_memory_buffer = new(memory_manager) MemBufInputSource(
				(const XMLByte*) dxl_string,
				strlen(dxl_string),
				"dxl test",
				false,
				memory_manager
	    	);

#ifdef GPOS_DEBUG
	CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG

	try
	{
		sax_2_xml_reader->parse(*input_src_memory_buffer);
	}
	catch (const XMLException&)
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLXercesParseError);
		return NULL;
	}
	catch (const SAXParseException&)
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLXercesParseError);
		return NULL;
	}
	catch (const SAXException&)
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLXercesParseError);
		return NULL;
	}


#ifdef GPOS_DEBUG
	CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG

	GPOS_CHECK_ABORT;

	// cleanup
	delete sax_2_xml_reader;
	delete input_src_memory_buffer;
	GPOS_DELETE(parse_handler_mgr);
	GPOS_DELETE(memory_manager);
	delete xsd_path;

	// reset time slice counter as unloading deleting Xerces SAX2 readers seems to take a lot of time (OPT-491)
#ifdef GPOS_DEBUG
	CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG

	GPOS_CHECK_ABORT;

	return parse_handler_dxl;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::GetParseHandlerForDXLFile
//
//	@doc:
//		Start the parsing of the given DXL string and return the top-level parser.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
CParseHandlerDXL *
CDXLUtils::GetParseHandlerForDXLFile
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_filename,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);
		
	// setup own memory manager
	CDXLMemoryManager mm(memory_pool);
	SAX2XMLReader* sax_2_xml_reader = NULL;
	{
		// we need to disable OOM simulation here, otherwise xerces throws ABORT signal
		CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
		CAutoTraceFlag atf2(EtraceSimulateAbort, false);

		sax_2_xml_reader = XMLReaderFactory::createXMLReader(&mm);
	}
	
	XMLCh *xsd_path = NULL;
	
	if (NULL != xsd_file_path)
	{
		// setup XSD validation
		sax_2_xml_reader->setFeature(XMLUni::fgSAX2CoreValidation, true);
		sax_2_xml_reader->setFeature(XMLUni::fgXercesDynamic, false);
		sax_2_xml_reader->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
		sax_2_xml_reader->setFeature(XMLUni::fgXercesSchema, true);
		
		sax_2_xml_reader->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
		sax_2_xml_reader->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);
		
		xsd_path = XMLString::transcode(xsd_file_path, &mm);
		sax_2_xml_reader->setProperty(XMLUni::fgXercesSchemaExternalSchemaLocation, (void*) xsd_path);
	}

	CParseHandlerManager phm(&mm, sax_2_xml_reader);
	CParseHandlerDXL *pph = CParseHandlerFactory::Pphdxl(memory_pool, &phm);
	phm.ActivateParseHandler(pph);
	GPOS_CHECK_ABORT;

	try
	{
		CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
		CAutoTraceFlag atf2(EtraceSimulateAbort, false);
		GPOS_CHECK_ABORT;

		sax_2_xml_reader->parse(dxl_filename);

		// reset time slice
#ifdef GPOS_DEBUG
	    CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG
	}
	catch (const XMLException&)
	{
		GPOS_DELETE(pph);
		delete sax_2_xml_reader;
		delete[] xsd_path;
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLXercesParseError);

		return NULL;
	}
	catch (const SAXParseException&ex)
	{
		GPOS_DELETE(pph);
		delete sax_2_xml_reader;
		delete[] xsd_path;
		
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLXercesParseError);

		return NULL;
	}
	catch (const SAXException&)
	{
		GPOS_DELETE(pph);
		delete sax_2_xml_reader;
		delete[] xsd_path;
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiDXLXercesParseError);

		return NULL;
	}

	GPOS_CHECK_ABORT;

	// cleanup
	delete sax_2_xml_reader;
	
	// reset time slice counter as unloading deleting Xerces SAX2 readers seems to take a lot of time (OPT-491)
#ifdef GPOS_DEBUG
    CWorker::Self()->ResetTimeSlice();
#endif // GPOS_DEBUG
    
	delete[] xsd_path;
	
	return pph;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::GetParseHandlerForDXLString
//
//	@doc:
//		Start the parsing of the given DXL string and return the top-level parser.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
CParseHandlerDXL *
CDXLUtils::GetParseHandlerForDXLString
	(
	IMemoryPool *memory_pool,
	const CWStringBase *dxl_string,
	const CHAR *xsd_file_path
	)
{
	CAutoRg<CHAR> a_sz;
	a_sz = CreateMultiByteCharStringFromWCString(memory_pool, dxl_string->GetBuffer());
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, a_sz.Rgt(), xsd_file_path);
	return parse_handler_dxl;
}



//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::GetPlanDXLNode
//
//	@doc:
//		Parse DXL string into a DXL plan tree.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
CDXLNode *
CDXLUtils::GetPlanDXLNode
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path,
	ULLONG *plan_id,
	ULLONG *plan_space_size
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != plan_id);
	GPOS_ASSERT(NULL != plan_space_size);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);
	
	GPOS_ASSERT(NULL != a_pphdxl.Value());
	
	// collect plan info from dxl parse handler
	CDXLNode *root_dxl_node = a_pphdxl->PdxlnPlan();
	*plan_id = a_pphdxl->UllPlanId();
	*plan_space_size = a_pphdxl->UllPlanSpaceSize();
	
	GPOS_ASSERT(NULL != root_dxl_node);
	
#ifdef GPOS_DEBUG
	root_dxl_node->Pdxlop()->AssertValid(root_dxl_node, true /* fValidateChildren */);
#endif
	
	root_dxl_node->AddRef();
	
	return root_dxl_node;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseQueryToQueryDXLTree
//
//	@doc:
//		Parse DXL string representing the query into
//		1. a DXL tree representing the query
//		2. a DXL tree representing the query output
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
CQueryToDXLResult *
CDXLUtils::ParseQueryToQueryDXLTree
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);

	// collect dxl tree of the query from dxl parse handler
	CDXLNode *root_dxl_node = parse_handler_dxl->PdxlnQuery();
	GPOS_ASSERT(NULL != root_dxl_node);
	
#ifdef GPOS_DEBUG
	root_dxl_node->Pdxlop()->AssertValid(root_dxl_node, true /* fValidateChildren */);
#endif
		
	root_dxl_node->AddRef();

	// collect the list of query output columns from the dxl parse handler
	GPOS_ASSERT(NULL != parse_handler_dxl->PdrgpdxlnOutputCols());
	DrgPdxln *pdrgpdxlnQO = parse_handler_dxl->PdrgpdxlnOutputCols();
	pdrgpdxlnQO->AddRef();

	// collect the list of CTEs
	DrgPdxln *cte_dxlnode_array = parse_handler_dxl->PdrgpdxlnCTE();
	GPOS_ASSERT(NULL != cte_dxlnode_array);
	cte_dxlnode_array->AddRef();

	CQueryToDXLResult *ptrOutput = GPOS_NEW(memory_pool) CQueryToDXLResult(root_dxl_node, pdrgpdxlnQO, cte_dxlnode_array);

	return ptrOutput;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToScalarExprDXLNode
//
//	@doc:
//		Parse a scalar expression as a top level node in a "ScalarExpr" tag.
//---------------------------------------------------------------------------
CDXLNode *
CDXLUtils::ParseDXLToScalarExprDXLNode
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CAutoP<CParseHandlerDXL> a_pphdxl(GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path));

	// collect dxl tree of the query from dxl parse handler
	CDXLNode *root_dxl_node = a_pphdxl->PdxlnScalarExpr();
	GPOS_ASSERT(NULL != root_dxl_node);
	root_dxl_node->AddRef();

	return root_dxl_node;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToIMDObjectArray
//
//	@doc:
//		Parse a list of metadata objects from the given DXL string.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
DrgPimdobj *
CDXLUtils::ParseDXLToIMDObjectArray
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);
	
	// collect metadata objects from dxl parse handler
	DrgPimdobj *imd_obj_array = parse_handler_dxl->Pdrgpmdobj();
	imd_obj_array->AddRef();
	
	return imd_obj_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToMDId
//
//	@doc:
//		Parse an mdid from a DXL metadata document
//
//---------------------------------------------------------------------------
IMDId *
CDXLUtils::ParseDXLToMDId
	(
	IMemoryPool *memory_pool,
	const CWStringBase *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);
	
	// collect metadata objects from dxl parse handler
	DrgPmdid *mdid_array = parse_handler_dxl->Pdrgpmdid();
	
	GPOS_ASSERT(1 == mdid_array->Size());
	
	IMDId *mdid = (*mdid_array)[0];
	mdid->AddRef();

	return mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToMDRequest
//
//	@doc:
//		Parse a metadata request.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
CMDRequest *
CDXLUtils::ParseDXLToMDRequest
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);

	// collect metadata ids from dxl parse handler
	CMDRequest *md_request = parse_handler_dxl->GetMiniDumper();
	GPOS_ASSERT(NULL != md_request);
	md_request->AddRef();

	return md_request;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToMDRequest
//
//	@doc:
//		Parse an MD request from the given DXL string.
//		Same as above but with a wide-character input
//
//---------------------------------------------------------------------------
CMDRequest *
CDXLUtils::ParseDXLToMDRequest
	(
	IMemoryPool *memory_pool,
	const WCHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	CAutoRg<CHAR> a_szDXL(CDXLUtils::CreateMultiByteCharStringFromWCString(memory_pool, dxl_string));
	
	// create and install a parse handler for the DXL document
	CMDRequest *md_request = ParseDXLToMDRequest(memory_pool, a_szDXL.Rgt(), xsd_file_path);

	return md_request;
}

// parse optimizer config DXL
COptimizerConfig *
CDXLUtils::ParseDXLToOptimizerConfig
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	// though we could access the traceflags member of the CParseHandlerDXL
	// here we don't access them or store them anywhere,
	// so, if using this function, note that any traceflags present
	// in the DXL being parsed will be discarded
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);

	// collect optimizer conf from dxl parse handler
	COptimizerConfig *optimizer_config = parse_handler_dxl->Poconf();
	GPOS_ASSERT(NULL != optimizer_config);
	optimizer_config->AddRef();

	return optimizer_config;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToStatsDerivedRelArray
//
//	@doc:
//		Parse a list of statistics objects from the given DXL string.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
DrgPdxlstatsderrel *
CDXLUtils::ParseDXLToStatsDerivedRelArray
	(
	IMemoryPool *memory_pool,
	const CWStringBase *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);

	// collect statistics objects from dxl parse handler
	DrgPdxlstatsderrel *dxl_derived_rel_stats_array = parse_handler_dxl->Pdrgpdxlstatsderrel();
	dxl_derived_rel_stats_array->AddRef();
	
	return dxl_derived_rel_stats_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToStatsDerivedRelArray
//
//	@doc:
//		Parse a list of statistics objects from the given DXL string.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
DrgPdxlstatsderrel *
CDXLUtils::ParseDXLToStatsDerivedRelArray
	(
	IMemoryPool *memory_pool,
	const CHAR *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);

	// collect statistics objects from dxl parse handler
	DrgPdxlstatsderrel *dxl_derived_rel_stats_array = parse_handler_dxl->Pdrgpdxlstatsderrel();
	dxl_derived_rel_stats_array->AddRef();
	
	return dxl_derived_rel_stats_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToOptimizerStatisticObjArray
//
//	@doc:
//		Translate the array of dxl statistics objects to an array of 
//		optimizer statistics object.
//---------------------------------------------------------------------------
CStatisticsArray *
CDXLUtils::ParseDXLToOptimizerStatisticObjArray
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	DrgPdxlstatsderrel *dxl_derived_rel_stats_array
	)
{
	GPOS_ASSERT(NULL != dxl_derived_rel_stats_array);

	CStatisticsArray *statistics_array = GPOS_NEW(memory_pool) CStatisticsArray(memory_pool);
	const ULONG ulRelStat = dxl_derived_rel_stats_array->Size();
	for (ULONG ulIdxRelStat = 0; ulIdxRelStat < ulRelStat; ulIdxRelStat++)
	{
		// create hash map from colid -> histogram
		HMUlHist *phmulhist = GPOS_NEW(memory_pool) HMUlHist(memory_pool);
		
		// width hash map
		HMUlDouble *phmuldouble = GPOS_NEW(memory_pool) HMUlDouble(memory_pool);
		
		CDXLStatsDerivedRelation *stats_derived_relation_dxl = (*dxl_derived_rel_stats_array)[ulIdxRelStat];
		const DrgPdxlstatsdercol *pdrgpdxlstatsdercol = stats_derived_relation_dxl->Pdrgpdxlstatsdercol();
		
		const ULONG ulColStats = pdrgpdxlstatsdercol->Size();
		for (ULONG ulIdxColStat = 0; ulIdxColStat < ulColStats; ulIdxColStat++)
		{
			CDXLStatsDerivedColumn *dxl_derived_col_stats = (*pdrgpdxlstatsdercol)[ulIdxColStat];
			
			ULONG ulColId = dxl_derived_col_stats->UlColId();
			CDouble dWidth = dxl_derived_col_stats->DWidth();
			CDouble dNullFreq = dxl_derived_col_stats->DNullFreq();
			CDouble dDistinctRemain = dxl_derived_col_stats->DDistinctRemain();
			CDouble dFreqRemain = dxl_derived_col_stats->DFreqRemain();
			
			DrgPbucket *pdrgppbucket = CDXLUtils::ParseDXLToBucketsArray(memory_pool, md_accessor, dxl_derived_col_stats);
			CHistogram *phist = GPOS_NEW(memory_pool) CHistogram(pdrgppbucket, true /*fWellDefined*/, dNullFreq, dDistinctRemain, dFreqRemain);
			
			phmulhist->Insert(GPOS_NEW(memory_pool) ULONG(ulColId), phist);
			phmuldouble->Insert(GPOS_NEW(memory_pool) ULONG(ulColId), GPOS_NEW(memory_pool) CDouble(dWidth));
		}
		
		CDouble dRows = stats_derived_relation_dxl->DRows();
		CStatistics *pstats = GPOS_NEW(memory_pool) CStatistics
										(
										memory_pool,
										phmulhist,
										phmuldouble,
										dRows,
										false /* fEmpty */
										);
		//pstats->AddCardUpperBound(memory_pool, ulIdxRelStat, dRows);

		statistics_array->Append(pstats);
	}
	
	return statistics_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToBucketsArray
//
//	@doc:
//		Extract the array of optimizer buckets from the dxl representation of
//		dxl buckets in the dxl derived column statistics object.
//---------------------------------------------------------------------------
DrgPbucket *
CDXLUtils::ParseDXLToBucketsArray
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	CDXLStatsDerivedColumn *dxl_derived_col_stats
	)
{
	DrgPbucket *pdrgppbucket = GPOS_NEW(memory_pool) DrgPbucket(memory_pool);
	
	const DrgPdxlbucket *buckets_dxl_array = dxl_derived_col_stats->Pdrgpdxlbucket();	
	const ULONG num_of_buckets = buckets_dxl_array->Size();
	for (ULONG ul = 0; ul < num_of_buckets; ul++)
	{
		CDXLBucket *bucket_dxl = (*buckets_dxl_array)[ul];
		
		// translate the lower and upper bounds of the bucket
		IDatum *pdatumLower = Pdatum(memory_pool, md_accessor, bucket_dxl->PdxldatumLower());
		CPoint *ppointLower = GPOS_NEW(memory_pool) CPoint(pdatumLower);
		
		IDatum *pdatumUpper = Pdatum(memory_pool, md_accessor, bucket_dxl->PdxldatumUpper());
		CPoint *ppointUpper = GPOS_NEW(memory_pool) CPoint(pdatumUpper);
		
		CBucket *pbucket = GPOS_NEW(memory_pool) CBucket
										(
										ppointLower,
										ppointUpper,
										bucket_dxl->FLowerClosed(),
										bucket_dxl->FUpperClosed(),
										bucket_dxl->DFrequency(),
										bucket_dxl->DDistinct()
										);
		
		pdrgppbucket->Append(pbucket);
	}
	
	return pdrgppbucket;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::Pdatum
//
//	@doc:
//		Translate the optimizer datum from dxl datum object
//
//---------------------------------------------------------------------------	
IDatum *
CDXLUtils::Pdatum
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	const CDXLDatum *dxl_datum
	 )
{
	IMDId *mdid = dxl_datum->Pmdid();
	return md_accessor->Pmdtype(mdid)->Pdatum(memory_pool, dxl_datum);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToIMDObjectArray
//
//	@doc:
//		Parse a list of metadata objects from the given DXL string.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
DrgPimdobj *
CDXLUtils::ParseDXLToIMDObjectArray
	(
	IMemoryPool *memory_pool,
	const CWStringBase *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CParseHandlerDXL *parse_handler_dxl = GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path);
	CAutoP<CParseHandlerDXL> a_pphdxl(parse_handler_dxl);
	
	// collect metadata objects from dxl parse handler
	DrgPimdobj *imd_obj_array = parse_handler_dxl->Pdrgpmdobj();
	imd_obj_array->AddRef();
	
	return imd_obj_array;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::ParseDXLToIMDIdCacheObj
//
//	@doc:
//		Parse a single metadata object given its DXL representation.
// 		Returns NULL if the DXL represents no metadata objects, or the first parsed
//		object if it does.
//		If a non-empty XSD schema location is provided, the DXL is validated against
//		that schema, and an exception is thrown if the DXL does not conform.
//
//---------------------------------------------------------------------------
IMDCacheObject *
CDXLUtils::ParseDXLToIMDIdCacheObj
	(
	IMemoryPool *memory_pool,
	const CWStringBase *dxl_string,
	const CHAR *xsd_file_path
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	// create and install a parse handler for the DXL document
	CAutoP<CParseHandlerDXL> a_pphdxl(GetParseHandlerForDXLString(memory_pool, dxl_string, xsd_file_path));
	
	// collect metadata objects from dxl parse handler
	DrgPimdobj *imd_obj_array = a_pphdxl->Pdrgpmdobj();

	if (0 == imd_obj_array->Size())
	{
		// no metadata objects found
		return NULL;
	}

	IMDCacheObject *pimdobjResult = (*imd_obj_array)[0];
	pimdobjResult->AddRef();
	
	return pimdobjResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeQuery
//
//	@doc:
//		Serialize a DXL Query tree into a DXL document
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeQuery
	(
	IMemoryPool *memory_pool,
	IOstream &os,
	const CDXLNode *dxl_query_node,
	const DrgPdxln *query_output_dxlnode_array,
	const DrgPdxln *cte_dxlnode_array,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != dxl_query_node && NULL != query_output_dxlnode_array);

	CAutoTimer at("\n[OPT]: DXL Query Serialization Time", GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	CXMLSerializer xml_serializer(memory_pool, os, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}
	
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenQuery));

	// serialize the query output columns
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenQueryOutput));
	for (ULONG ul = 0; ul < query_output_dxlnode_array->Size(); ++ul)
	{
		CDXLNode *scalar_ident = (*query_output_dxlnode_array)[ul];
		scalar_ident->SerializeToDXL(&xml_serializer);
	}
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenQueryOutput));

	// serialize the CTE list
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCTEList));
	const ULONG ulCTEs = cte_dxlnode_array->Size();
	for (ULONG ul = 0; ul < ulCTEs; ++ul)
	{
		CDXLNode *cte = (*cte_dxlnode_array)[ul];
		cte->SerializeToDXL(&xml_serializer);
	}
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCTEList));

	
	dxl_query_node->SerializeToDXL(&xml_serializer);

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenQuery));
	
	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeULLONG
//
//	@doc:
//		Serialize a ULLONG value
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeULLONG
	(
	IMemoryPool *memory_pool,
	ULLONG value
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	CAutoTraceFlag auto_trace_flg(EtraceSimulateAbort, false);

	CAutoP<CWStringDynamic> string_var(GPOS_NEW(memory_pool) CWStringDynamic(memory_pool));

	// create a string stream to hold the result of serialization
	COstreamString oss(string_var.Value());
	oss << value;

	return string_var.Reset();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializePlan
//
//	@doc:
//		Serialize a DXL tree into a DXL document
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializePlan
	(
	IMemoryPool *memory_pool,
	IOstream& os,
	const CDXLNode *node,
	ULLONG plan_id,
	ULLONG plan_space_size,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != node);

	CAutoTimer at("\n[OPT]: DXL Plan Serialization Time", GPOS_FTRACE(EopttracePrintOptimizationStatistics));

	CXMLSerializer xml_serializer(memory_pool, os, indentation);
	
	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}
	
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenPlan));

	// serialize plan id and space size attributes

	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenPlanId), plan_id);
	xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenPlanSpaceSize), plan_space_size);

	node->SerializeToDXL(&xml_serializer);

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenPlan));
	
	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeMetadata
//
//	@doc:
//		Serialize a list of MD objects into a DXL document and write to
//		to the provided output stream
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeMetadata
	(
	IMemoryPool *memory_pool,
	const DrgPimdobj *imd_obj_array,
	IOstream &os,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != imd_obj_array);

	CXMLSerializer xml_serializer(memory_pool, os, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}
	
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMetadata));


	for (ULONG ul = 0; ul < imd_obj_array->Size(); ul++)
	{
		IMDCacheObject *imd_cache_obj = (*imd_obj_array)[ul];
		imd_cache_obj->Serialize(&xml_serializer);
	}

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMetadata));

	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}
	
	return;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeMetadata
//
//	@doc:
//		Serialize a list of MD objects into a DXL document and write to
//		to the provided output stream
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeMetadata
	(
	IMemoryPool *memory_pool,
	const IMDId *mdid,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(mdid->IsValid());

	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);
	
	// create a string stream to hold the result of serialization
	COstreamString oss(dxl_string);
	
	CXMLSerializer xml_serializer(memory_pool, oss, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}
	
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMetadata));
	
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
									CDXLTokens::PstrToken(EdxltokenMdid));				
	mdid->Serialize(&xml_serializer, CDXLTokens::PstrToken(EdxltokenValue));
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), 
					CDXLTokens::PstrToken(EdxltokenMdid));
	
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMetadata));

	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}
	
	return dxl_string;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeSamplePlans
//
//	@doc:
//		Serialize a list of sample plans in the given enumerator config
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeSamplePlans
	(
	IMemoryPool *memory_pool,
	CEnumeratorConfig *enumerator_cfg,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	// create a string stream to hold the result of serialization
	COstreamString oss(dxl_string);

	CXMLSerializer xml_serializer(memory_pool, oss, indentation);
	SerializeHeader(memory_pool, &xml_serializer);

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenSamplePlans));

	const ULONG size = enumerator_cfg->UlCreatedSamples();
	for (ULONG ul = 0; ul < size; ul++)
	{
		xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenSamplePlan));
		// we add 1 to plan id since id's are zero-based internally, and we reserve 0 for best plan
		xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenPlanId), enumerator_cfg->UllPlanSample(ul) + 1);
		xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenRelativeCost), enumerator_cfg->CostPlanSample(ul) / enumerator_cfg->CostBest());
		xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenSamplePlan));
	}

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenSamplePlans));

	SerializeFooter(&xml_serializer);

	return dxl_string;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeCostDistr
//
//	@doc:
//		Serialize cost distribution
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeCostDistr
	(
	IMemoryPool *memory_pool,
	CEnumeratorConfig *enumerator_cfg,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);

	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	// create a string stream to hold the result of serialization
	COstreamString oss(dxl_string);

	CXMLSerializer xml_serializer(memory_pool, oss, indentation);
	SerializeHeader(memory_pool, &xml_serializer);

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostDistr));

	const ULONG size = enumerator_cfg->UlCostDistrSize();
	for (ULLONG ul = 0; ul < size; ul++)
	{
		xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenValue));
		xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenX), enumerator_cfg->DCostDistrX(ul));
		xml_serializer.AddAttribute(CDXLTokens::PstrToken(EdxltokenY), enumerator_cfg->DCostDistrY(ul));
		xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenValue));
	}

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenCostDistr));

	SerializeFooter(&xml_serializer);

	return dxl_string;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeMDRequest
//
//	@doc:
//		Serialize a list of mdids into a DXL MD Request document and write to
//		to the provided output stream
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeMDRequest
	(
	IMemoryPool *memory_pool,
	CMDRequest *md_request,
	IOstream &os,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != md_request);

	CXMLSerializer xml_serializer(memory_pool, os, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}

	md_request->Serialize(&xml_serializer);

	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}

	return;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeMDRequest
//
//	@doc:
//		Serialize a list of mdids into a DXL MD Request document and write to
//		to the provided output stream
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeMDRequest
	(
	IMemoryPool *memory_pool,
	const IMDId *mdid,
	IOstream &os,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != mdid);

	CXMLSerializer xml_serializer(memory_pool, os, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMDRequest));

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMdid));
	mdid->Serialize(&xml_serializer, CDXLTokens::PstrToken(EdxltokenValue));
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMdid));

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMDRequest));

	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}

	return;
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeStatistics
//
//	@doc:
//		Serialize a list of statistics objects into a DXL document
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeStatistics
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor, 
	const CStatisticsArray *statistics_array,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != statistics_array);
	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	// create a string stream to hold the result of serialization
	COstreamString oss(dxl_string);

	CDXLUtils::SerializeStatistics(memory_pool, md_accessor, statistics_array, oss, serialize_header_footer, indentation);

	return dxl_string;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeStatistics
//
//	@doc:
//		Serialize a list of statistics objects into a DXL document and write to
//		to the provided output stream
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeStatistics
	(
	IMemoryPool *memory_pool,
	CMDAccessor *md_accessor,
	const CStatisticsArray *statistics_array,
	IOstream &os,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != statistics_array);

	CXMLSerializer xml_serializer(memory_pool, os, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}

	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenStatistics));

	GPOS_ASSERT(NULL != statistics_array);

	for (ULONG ul = 0; ul < statistics_array->Size(); ul++)
	{
		CStatistics *pstats = (*statistics_array)[ul];
		CDXLStatsDerivedRelation *stats_derived_relation_dxl = pstats->Pdxlstatsderrel(memory_pool, md_accessor);
		stats_derived_relation_dxl->Serialize(&xml_serializer);
		stats_derived_relation_dxl->Release();
	}

	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenStatistics));

	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}

	return;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeMetadata
//
//	@doc:
//		Serialize a list of MD objects into a DXL document
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeMetadata
	(
	IMemoryPool *memory_pool,
	const DrgPimdobj *imd_obj_array,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != imd_obj_array);
	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);
	
	// create a string stream to hold the result of serialization
	COstreamString oss(dxl_string);
	
	CDXLUtils::SerializeMetadata(memory_pool, imd_obj_array, oss, serialize_header_footer, indentation);

	return dxl_string;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeMDObj
//
//	@doc:
//		Serialize an MD object into a DXL document
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeMDObj
	(
	IMemoryPool *memory_pool,
	const IMDCacheObject *imd_cache_obj,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != imd_cache_obj);
	CAutoTraceFlag atf2(EtraceSimulateAbort, false);

	CAutoP<CWStringDynamic> string_var(GPOS_NEW(memory_pool) CWStringDynamic(memory_pool));
	
	// create a string stream to hold the result of serialization
	COstreamString oss(string_var.Value());

	CXMLSerializer xml_serializer(memory_pool, oss, indentation);
	
	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
		xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMetadata));
	}
	GPOS_CHECK_ABORT;
	
	imd_cache_obj->Serialize(&xml_serializer);
	GPOS_CHECK_ABORT;
		
	if (serialize_header_footer)
	{
		xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenMetadata));
		SerializeFooter(&xml_serializer);
	}
	
	GPOS_CHECK_ABORT;
	return string_var.Reset();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeScalarExpr
//
//	@doc:
//		Serialize a DXL tree representing a ScalarExpr into a DXL document
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::SerializeScalarExpr
	(
	IMemoryPool *memory_pool,
	const CDXLNode *node,
	BOOL serialize_header_footer,
	BOOL indentation
	)
{
	GPOS_ASSERT(NULL != memory_pool);
	GPOS_ASSERT(NULL != node);
	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	// create a string stream to hold the result of serialization
	COstreamString oss(dxl_string);
	CXMLSerializer xml_serializer(memory_pool, oss, indentation);

	if (serialize_header_footer)
	{
		SerializeHeader(memory_pool, &xml_serializer);
	}
	xml_serializer.OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarExpr));

	// serialize the content of the scalar expression
	node->SerializeToDXL(&xml_serializer);
	xml_serializer.CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenScalarExpr));
	if (serialize_header_footer)
	{
		SerializeFooter(&xml_serializer);
	}
	return dxl_string;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeHeader
//
//	@doc:
//		Serialize the DXL document header
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeHeader
	(
	IMemoryPool *memory_pool,
	CXMLSerializer *xml_serializer
	)
{
	GPOS_ASSERT(NULL != xml_serializer);
	
	xml_serializer->StartDocument();
	
	xml_serializer->OpenElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDXLMessage));
	
	// add namespace specification xmlns:dxl="...."
	CWStringDynamic namespace_specification_string(memory_pool);
	namespace_specification_string.AppendFormat
						(
						GPOS_WSZ_LIT("%ls%ls%ls"),
						CDXLTokens::PstrToken(EdxltokenNamespaceAttr)->GetBuffer(),
						CDXLTokens::PstrToken(EdxltokenColon)->GetBuffer(),
						CDXLTokens::PstrToken(EdxltokenNamespacePrefix)->GetBuffer()
						);
	
	xml_serializer->AddAttribute(&namespace_specification_string, CDXLTokens::PstrToken(EdxltokenNamespaceURI));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeFooter
//
//	@doc:
//		Serialize the DXL document footer
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeFooter
	(
	CXMLSerializer *xml_serializer
	)
{
	GPOS_ASSERT(NULL != xml_serializer);
	
	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix), CDXLTokens::PstrToken(EdxltokenDXLMessage));
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::CreateDynamicStringFromXMLChArray
//
//	@doc:
//		Create a GPOS string object from a Xerces XMLCh* string.
//		The function allocates the returned string in the provided memory pool
//		and it is the responsibility of the caller to release it.
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::CreateDynamicStringFromXMLChArray
	(
	CDXLMemoryManager *memory_manager,
	const XMLCh *xml_string
	)
{
	GPOS_ASSERT(NULL != memory_manager);
	GPOS_ASSERT(NULL != xml_string);
	
	IMemoryPool *memory_pool = memory_manager->Pmp();
	
	{
		CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
		CHAR *sz = XMLString::transcode(xml_string, memory_manager);

		CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);
		dxl_string->AppendFormat(GPOS_WSZ_LIT("%s"), sz);
	
		// cleanup temporary buffer
		XMLString::release(&sz, memory_manager);
	
		return dxl_string;
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::PstrFromBase64XMLStr
//
//	@doc:
//		Create a decoded byte array from a base 64 encoded XML string.
//
//---------------------------------------------------------------------------
BYTE *
CDXLUtils::CreateStringFrom64XMLStr
	(
	CDXLMemoryManager *memory_manager,
	const XMLCh *xml_string,
	ULONG *length // output: length of constructed byte array
	)
{
	GPOS_ASSERT(NULL != memory_manager);
	GPOS_ASSERT(NULL != xml_string);

	CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
	IMemoryPool *memory_pool = memory_manager->Pmp();

	// find out xml string length
	ULONG len = XMLString::stringLen(xml_string);

	// convert XML string into array of XMLByte
	CAutoRg<XMLByte> data_in_byte;
	data_in_byte = (XMLByte *) GPOS_NEW_ARRAY(memory_pool, XMLByte, len + 1);
	for (ULONG ul = 0; ul < len; ul++)
	{
		GPOS_ASSERT(xml_string[ul] <= 256 && "XML string not in Base64 encoding");

		data_in_byte[ul] = (XMLByte) xml_string[ul];
	}
	data_in_byte[len] = 0;

	// decode string
	XMLSize_t xml_size = 0;
	XMLByte *xml_byte_out = Base64::decode(data_in_byte.Rgt(), &xml_size, memory_manager);
	*length = static_cast<ULONG>(xml_size);

	return static_cast<BYTE *>(xml_byte_out);
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::CreateDynamicStringFromCharArray
//
//	@doc:
//		Create a GPOS string object from a character array.
//		The function allocates the returned string in the provided memory pool
//		and it is the responsibility of the caller to release it.
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::CreateDynamicStringFromCharArray
	(
	IMemoryPool *memory_pool,
	const CHAR *sz
	)
{
	GPOS_ASSERT(NULL != sz);
	
	CAutoP<CWStringDynamic> string_var(GPOS_NEW(memory_pool) CWStringDynamic(memory_pool));
	string_var->AppendFormat(GPOS_WSZ_LIT("%s"), sz);
	return string_var.Reset();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::CreateMDNameFromCharArray
//
//	@doc:
//		Create a GPOS string object from a character array.
//		The function allocates the returned string in the provided memory pool
//		and it is the responsibility of the caller to release it.
//
//---------------------------------------------------------------------------
CMDName *
CDXLUtils::CreateMDNameFromCharArray
	(
	IMemoryPool *memory_pool,
	const CHAR *sz
	)
{
	GPOS_ASSERT(NULL != sz);
	
	CWStringDynamic *dxl_string = CDXLUtils::CreateDynamicStringFromCharArray(memory_pool, sz);
	CMDName *md_name = GPOS_NEW(memory_pool) CMDName(memory_pool, dxl_string);

	// CMDName ctor created a copy of the string
	GPOS_DELETE(dxl_string);
	
	return md_name;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::PmdnameFromXmlsz
//
//	@doc:
//		Create a GPOS string object from a Xerces character array.
//		The function allocates the returned string in the provided memory pool
//		and it is the responsibility of the caller to release it.
//
//---------------------------------------------------------------------------
CMDName *
CDXLUtils::PmdnameFromXmlsz
	(
	CDXLMemoryManager *memory_manager,
	const XMLCh *xml_string
	)
{
	GPOS_ASSERT(NULL != xml_string);
	
	CHAR *sz = XMLString::transcode(xml_string, memory_manager);
	CMDName *md_name = CreateMDNameFromCharArray(memory_manager->Pmp(), sz);
	
	// cleanup temporary buffer
	XMLString::release(&sz, memory_manager);
	
	return md_name;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::EncodeByteArrayToString
//
//	@doc:
//		Use Base64 encoding to convert bytearray to a string.
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::EncodeByteArrayToString
	(
	IMemoryPool *memory_pool,
	const BYTE *byte,
	ULONG length
	)
{
	CAutoP<CDXLMemoryManager> a_pmm(GPOS_NEW(memory_pool) CDXLMemoryManager(memory_pool));
	CAutoP<CWStringDynamic> string_var(GPOS_NEW(memory_pool) CWStringDynamic(memory_pool));

	GPOS_ASSERT(length > 0);

	XMLSize_t output_length = 0;
	const XMLByte *input = (const XMLByte *) byte;

	XMLSize_t input_length = (XMLSize_t) length;

	CAutoRg<XMLByte> xml_byte_buffer;

	{
		CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
		xml_byte_buffer = Base64::encode(input, input_length, &output_length, a_pmm.Value());
	}

	GPOS_ASSERT(NULL != xml_byte_buffer.Rgt());

	// assert that last byte is 0
	GPOS_ASSERT(0 == xml_byte_buffer[output_length]);

	// there may be padded bytes. We don't need them. We zero out there bytes.
#ifdef GPOS_DEBUG
	ULONG ulNewLength = output_length;
#endif  // GPOS_DEBUG
	while (('\n' == xml_byte_buffer[output_length]
			|| 0 == xml_byte_buffer[output_length])
			&& 0 < output_length)
	{
		xml_byte_buffer[output_length] = 0;
#ifdef GPOS_DEBUG
		ulNewLength = output_length;
#endif  // GPOS_DEBUG
		output_length--;
	}
	GPOS_ASSERT(0 == xml_byte_buffer[ulNewLength]);

	CHAR *return_buffer = (CHAR *) (xml_byte_buffer.Rgt());
	string_var->AppendCharArray(return_buffer);

	return string_var.Reset();
}


//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::DecodeByteArrayFromString
//
//	@doc:
//		Decode byte array from Base64 encoded string.
//
//---------------------------------------------------------------------------
BYTE *
CDXLUtils::DecodeByteArrayFromString
	(
	IMemoryPool *memory_pool,
	const CWStringDynamic *dxl_string,
	ULONG *length
	)
{
	CAutoP<CDXLMemoryManager> a_pmm(GPOS_NEW(memory_pool) CDXLMemoryManager(memory_pool));

	XMLSize_t xml_size = 0;

	const WCHAR *pwc = dxl_string->GetBuffer();

	// We know that the input is encoded using Base64.
	XMLSize_t input_length = dxl_string->Length();

	CAutoRg<XMLByte> data_in_byte;

	data_in_byte = (XMLByte*) GPOS_NEW_ARRAY(memory_pool, XMLByte, input_length+1);

	for (XMLSize_t i = 0; i < input_length; i++)
	{
		GPOS_ASSERT(pwc[i] <= 256);
		data_in_byte[i] = (XMLByte) pwc[i];
	}

	data_in_byte[input_length] = 0;

	XMLByte *xml_byte = NULL;
	{
		CAutoTraceFlag auto_trace_flg(EtraceSimulateOOM, false);
		xml_byte = Base64::decode(data_in_byte.Rgt(), &xml_size, a_pmm.Value());
	}

	(* length) = static_cast<ULONG>(xml_size);

	return static_cast<BYTE *>(xml_byte);
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::Serialize
//
//	@doc:
//		Serialize a list of unsigned integers into a comma-separated string
//
//---------------------------------------------------------------------------
CWStringDynamic *
CDXLUtils::Serialize
	(
	IMemoryPool *memory_pool,
	const ULongPtrArray2D *ulong_ptr_array_2D
	)
{
	const ULONG len = ulong_ptr_array_2D->Size();
	CWStringDynamic *pstrKeys = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);
	for (ULONG ul = 0; ul < len; ul++)
	{
		ULongPtrArray *pdrgpul = (*ulong_ptr_array_2D)[ul];
		CWStringDynamic *key_set_string = CDXLUtils::Serialize(memory_pool, pdrgpul);

		pstrKeys->Append(key_set_string);

		if (ul < len - 1)
		{
			pstrKeys->AppendFormat(GPOS_WSZ_LIT("%ls"), GPOS_WSZ_LIT(";"));
		}

		GPOS_DELETE(key_set_string);
	}

	return pstrKeys;
}

// Serialize a list of chars into a comma-separated string
CWStringDynamic *
CDXLUtils::SerializeToCommaSeparatedString
	(
	IMemoryPool *memory_pool,
	const CharPtrArray *char_ptr_array
	)
{
	CWStringDynamic *dxl_string = GPOS_NEW(memory_pool) CWStringDynamic(memory_pool);

	ULONG length = char_ptr_array->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CHAR value = *((*char_ptr_array)[ul]);
		if (ul == length - 1)
		{
			// last element: do not print a comma
			dxl_string->AppendFormat(GPOS_WSZ_LIT("%c"), value);
		}
		else
		{
			dxl_string->AppendFormat(GPOS_WSZ_LIT("%c%ls"), value, CDXLTokens::PstrToken(EdxltokenComma)->GetBuffer());
		}
	}

	return dxl_string;
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::CreateMultiByteCharStringFromWCString
//
//	@doc:
//		Converts a wide character string into a character array in the provided memory pool
//
//---------------------------------------------------------------------------
CHAR *
CDXLUtils::CreateMultiByteCharStringFromWCString
	(
	IMemoryPool *memory_pool,
	const WCHAR *wc_string
	)
{
	GPOS_ASSERT(NULL != wc_string);

	ULONG max_length = GPOS_WSZ_LENGTH(wc_string) * GPOS_SIZEOF(WCHAR) + 1;
	CHAR *sz = GPOS_NEW_ARRAY(memory_pool, CHAR, max_length);
	CAutoRg<CHAR> a_sz(sz);

#ifdef GPOS_DEBUG
	INT i = (INT)
#endif
	wcstombs(sz, wc_string, max_length);
	GPOS_ASSERT(0 <= i);

	sz[max_length - 1] = '\0';

	return a_sz.RgtReset();
}

//---------------------------------------------------------------------------
//		CDXLUtils::Read
//
//	@doc:
//		Read a given text file in a character buffer.
//		The function allocates memory from the provided memory pool, and it is
//		the responsibility of the caller to deallocate it.
//
//---------------------------------------------------------------------------
CHAR *
CDXLUtils::Read
	(
	IMemoryPool *memory_pool,
	const CHAR *filename
	)
{
	GPOS_TRACE_FORMAT("opening file %s", filename);

	CFileReader fr;
	fr.Open(filename);

	ULONG_PTR file_size = (ULONG_PTR) fr.FileSize();
	CAutoRg<CHAR> read_buffer(GPOS_NEW_ARRAY(memory_pool, CHAR, file_size + 1));
	
	ULONG_PTR read_bytes = fr.ReadBytesToBuffer((BYTE *) read_buffer.Rgt(), file_size);
	fr.Close();
	
	GPOS_ASSERT(read_bytes == file_size);
	
	read_buffer[read_bytes] = '\0';
		
	return read_buffer.RgtReset();
}

//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::SerializeBound
//
//	@doc:
//		Serialize a datum with a given tag. Result xml looks like
// 		<tag>
//			<const.../>
//		</tag>
//
//---------------------------------------------------------------------------
void
CDXLUtils::SerializeBound
	(
	IDatum *pdatum,
	const CWStringConst *dxl_string,
	CXMLSerializer *xml_serializer
	)
{
	xml_serializer->OpenElement
				(
				CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
				dxl_string
				);

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDType *md_type = md_accessor->Pmdtype(pdatum->Pmdid());
	CDXLScalarConstValue *pdxlop = md_type->PdxlopScConst(xml_serializer->Pmp(), pdatum);
	pdxlop->SerializeToDXL(xml_serializer, NULL);

	xml_serializer->CloseElement(CDXLTokens::PstrToken(EdxltokenNamespacePrefix),
						dxl_string);
	pdxlop->Release();
}

#ifdef GPOS_DEBUG
//---------------------------------------------------------------------------
//	@function:
//		CDXLUtils::DebugPrintMDIdArray
//
//	@doc:
//		Print an array of mdids
//
//---------------------------------------------------------------------------
void
CDXLUtils::DebugPrintMDIdArray
	(
	IOstream &os,
	DrgPmdid *mdid_array
	)
{
	ULONG len = mdid_array->Size();
	for (ULONG ul = 0; ul < len; ul++)
	{
		const IMDId *mdid = (*mdid_array)[ul];
		mdid->OsPrint(os);
		os << " ";
	}

	os << std::endl;
}
#endif

// EOF

