#include "util.hpp"
#include <vector>
#include <assert.h>
#include <queue>
#include <stack>
#include <unordered_set>
#include <utility> // std::pair
#include <algorithm> // std::min

extern "C"
{
#include <postgres.h>
#include <catalog/pg_type_d.h>
#include <utils/array.h>
#include <funcapi.h>

#include <nodes/print.h>
#include <nodes/plannodes.h>
#include <nodes/execnodes.h>

#include <executor/spi.h>
#include <executor/executor.h>

#include <utils/snapmgr.h>
#include <optimizer/cost.h>    


#include <access/table.h>
#include <access/tableam.h>
#include <access/relation.h>

#include <access/genam.h>
#include <access/relscan.h>
#include <access/printtup.h>

#include <commands/vacuum.h>
#include <miscadmin.h>
#include <utils/elog.h>
#include <utils/rel.h>
#include <utils/selfuncs.h>

#include "libpq/pqformat.h"
#include <utils/builtins.h>
}

extern "C"
{
    PGDLLEXPORT PG_FUNCTION_INFO_V1(topk);
}


#ifdef NDEBUG
#define DEBUG_PRINT(...) ;
#else
#define DEBUG_PRINT(...) do{ elog(WARNING, __VA_ARGS__); } while (false)
#endif

inline float castAttrToFloat(TupleTableSlot *slot, int attno){
	bool isNull;
	Datum value = slot_getattr(slot, attno, &isNull);
	float retval = 0;
	
	if ( isNull)
		return 0;
	
	TupleDesc tupdesc = slot->tts_tupleDescriptor;
	Form_pg_attribute attr = TupleDescAttr(tupdesc, attno-1);
	switch (attr->atttypid)	 {
	case INT4OID:
		retval = (float)DatumGetInt32(value);
		break;

	case INT8OID:
		retval = (float)DatumGetInt64(value);
		break;
		
	case FLOAT8OID:
		retval = (float)DatumGetFloat8(value);
		break;

	default:
		elog(ERROR, "castFloat unsupported oid = %d", attr->atttypid);

	}
	return retval;
}

bool
printtupleslot(TupleTableSlot *slot)
{
	TupleDesc	tupdesc = slot->tts_tupleDescriptor;
	int			i;

	/* Make sure the tuple is fully deconstructed */
	slot_getallattrs(slot);

	char buf[8192];
	uint32 buflen = 0;
	int incr=0;

	for (i = 0; i < tupdesc->natts; ++i){

		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
		Datum		value;

		if (slot->tts_isnull[i]){
			incr = snprintf(buf+buflen, sizeof(buf)-buflen, "%10s | ", "null");
			buflen+=incr;
			continue;
		}

		value = slot->tts_values[i];

		/*
		 * We can't call the regular type output functions here because we
		 * might not have catalog access.  Instead, we must hard-wire
		 * knowledge of the required types.
		 */
		switch (attr->atttypid)	 {
		case TEXTOID:
			{
				text	   *t = DatumGetTextPP(value);
				incr = snprintf(buf+buflen, sizeof(buf)-buflen, "%10s | ",
								text_to_cstring(t)
								);
				buflen+=incr;

			}
			break;

		case INT4OID:
			{
				int32		num = DatumGetInt32(value);
				incr = snprintf(buf+buflen, sizeof(buf)-buflen, "%10d | ",
								num
								);
				buflen+=incr;
			}
			break;

		case INT8OID:
			{
				int64		num = DatumGetInt64(value);

				incr = snprintf(buf+buflen, sizeof(buf)-buflen, "%10ld | ",
								num
								);
				buflen+=incr;
			}
			break;
			
		case FLOAT8OID:
			{
				float		num = DatumGetFloat8(value);

				incr = snprintf(buf+buflen, sizeof(buf)-buflen, "%10f | ",
								num
								);
				buflen+=incr;
			}
			break;

		case BPCHAROID:
			{
				BpChar*		bp = DatumGetBpCharPP(value);

				incr = snprintf(buf+buflen, sizeof(buf)-buflen, "%10s | ",
								text_to_cstring((text*) bp)
								);
				buflen+=incr;
			}
			break;

		default:
			incr = snprintf(buf+buflen, sizeof(buf)-buflen, "oid = %u | ",
							attr->atttypid
							);
			buflen+=incr;
			// elog(ERROR, "unsupported type OID: %u", attr->atttypid);
		}
		
	}
	elog(WARNING, buf);

	return true;
}


inline Oid getrelid(const std::string relname){
	char cmd[256];
	int cmd_len = snprintf(cmd, sizeof(cmd), "SELECT oid FROM pg_catalog.pg_class WHERE relname = '%s';", relname.c_str());
	
	if ( cmd_len < 0 ) {
		elog(ERROR, "getrelid:: relname '%s' is too long", relname.c_str());
		return -1;
	}

	// DEBUG_PRINT("cmd string = %s", cmd);

	SPI_connect();
	int ret = SPI_exec(cmd, 1);

	uint64 proc = SPI_processed;

	Oid retval = -1;

	if ( ret > 0 && SPI_tuptable != NULL ){
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		SPITupleTable *tuptable = SPI_tuptable;

		if ( proc > 1 )
			ereport(ERROR,
					(errcode(ERRCODE_DATA_EXCEPTION), errmsg("only a single relname should be returned")));
		
		if ( tupdesc->natts != 1 )
			ereport(ERROR,
					(errcode(ERRCODE_DATA_EXCEPTION), errmsg("only a single attribute should be returned")));


		if ( proc == 1 ) {
			HeapTuple tuple = tuptable->vals[0];
			bool isnull;
			Datum value = SPI_getbinval(tuple, tupdesc, 1, &isnull);
			if ( isnull ){
				ereport(ERROR,
						(errcode(ERRCODE_DATA_EXCEPTION), errmsg("Non-null value should be returned")));
			}
			retval = DatumGetObjectId(value);
			DEBUG_PRINT("getrelid:: found relid: relname (%s) oid = %d", relname.c_str(), retval);
		} else {
			elog(ERROR, "getrelid:: cannot find relid for relname (%s)", relname.c_str());
		}
	}

	SPI_finish();
	return retval;
}

// Dfs of execution tree for execution node of IndexScan
IndexScanState* findIndexScanState(PlanState* node){
	IndexScanState *retval = NULL;
	if (!node)
		return retval;

	if (nodeTag(node) == T_IndexScanState)
		return (IndexScanState*)node;

	retval = findIndexScanState(outerPlanState(node));
	if (retval)
		return retval;
	
	return findIndexScanState(innerPlanState(node));
}

// Dfs of plan tree for plan node of IndexScan
IndexScan* findIndexScanNode(Plan* node){
	IndexScan *retval = NULL;
	if (!node)
		return retval;

	if (nodeTag(node) == T_IndexScan)
		return (IndexScan*)node;

	retval = findIndexScanNode(outerPlan(node));
	if (retval)
		return retval;
	
	return findIndexScanNode(innerPlan(node));
}

// find the sql plan which contains the indexscan node
// return NULL if not found
PlannedStmt* extractIndexScanNode(char* query){
	CachedPlan *cplan = NULL;

	// store old value of enable_seqscan, and set it to false to enforce index_scan
	bool old_enable_seqscan = enable_seqscan;
	enable_seqscan=false;

	PlannedStmt* plan= NULL;

	// parse->plan->optimize a simple sql query that has the "order by" expression
	SPIPlanPtr planptr = SPI_prepare(query, 0, NULL);
	cplan = SPI_plan_get_cached_plan(planptr);
	List* stmt_list = cplan->stmt_list;

	ListCell* lc;
	IndexScan* node = NULL;
	foreach(lc, stmt_list){
		PlannedStmt *stmt = lfirst_node(PlannedStmt, lc);
		// elog_node_display(WARNING, NULL, stmt, true);
		if ( (node = findIndexScanNode(stmt->planTree)) != NULL ){
			plan = stmt;
			// elog_node_display(WARNING, "INDEXSCAN", node, true);
		};
	}

	// restore old value of enable_seqscan
	enable_seqscan = old_enable_seqscan;

	return plan;
}



struct ItemPointerDataHash {
	std::size_t operator()(const ItemPointerData& key) const
	{
		return ((((size_t)key.ip_blkid.bi_hi)<<32) +
				(key.ip_blkid.bi_lo<<16) +
				key.ip_posid);
	}
};

inline uint64_t castToUint64(const ItemPointerData& key) 
{
	return (((uint64_t)key.ip_blkid.bi_hi)<<32) |
		(((uint64_t)key.ip_blkid.bi_lo)<<16) | (uint64_t)key.ip_posid;
}


struct ItemPointerDataEqual {
	bool operator()(const ItemPointerData& lhs, const ItemPointerData& rhs) const
	{
		return (lhs.ip_blkid.bi_hi == rhs.ip_blkid.bi_hi &&
				lhs.ip_blkid.bi_lo == rhs.ip_blkid.bi_lo &&
				lhs.ip_posid == rhs.ip_posid);
	}
};

// typedef std::pair<float, ItemPointerData> pq_item;
typedef std::pair<float, HeapTuple> pq_item;

struct pq_item_compare_gt
{
	bool operator()(const pq_item& lhs, const pq_item& rhs)
	{
		return lhs.first > rhs.first;
	}
};

struct pq_item_compare_lt
{
	bool operator()(const pq_item& lhs, const pq_item& rhs)
	{
		return lhs.first < rhs.first;
	}
};

TupleDesc copyAndDropLastAttr(TupleDesc tupdesc, bool doDrop){
	DEBUG_PRINT("%s enter in = %p, doDrop = %d", __func__, tupdesc, doDrop);
	TupleDesc	desc;
	int			i;

	desc = CreateTemplateTupleDesc(doDrop? std::max(tupdesc->natts-1, 0): tupdesc->natts);

	/* Flat-copy the attribute array */
	memcpy(TupleDescAttr(desc, 0),
		   TupleDescAttr(tupdesc, 0),
		   desc->natts * sizeof(FormData_pg_attribute));

	/*
	 * Since we're not copying constraints and defaults, clear fields
	 * associated with them.
	 */
	for (i = 0; i < desc->natts; i++)
	{
		Form_pg_attribute att = TupleDescAttr(desc, i);

		att->attnotnull = false;
		att->atthasdef = false;
		att->atthasmissing = false;
		att->attidentity = '\0';
		att->attgenerated = '\0';
	}

	/* We can copy the tuple type identification, too */
	desc->tdtypeid = tupdesc->tdtypeid;
	desc->tdtypmod = tupdesc->tdtypmod;

	DEBUG_PRINT("%s newdesc->natts = %d", __func__, desc->natts);
	DEBUG_PRINT("%s exit", __func__);
	return desc;
}

inline HeapTuple copyAndDropLastItem(TupleTableSlot* slot){
	HeapTuple retval = ExecCopySlotHeapTuple(slot);
	return retval;	
}

typedef struct{
	PlanState ps;
	uint32_t topk;
	uint32_t ef;
	uint32_t term_cond;
	bool finish;
	std::priority_queue<pq_item, std::vector<pq_item>, pq_item_compare_lt> *proc_pq;
	std::unordered_set<uint64_t> *seenSet;

	std::stack<pq_item> *result_stack;
	
	IndexFetchTableData *result_heap_scan;
	Relation relation;
	std::vector<IndexScanState*> *children;
	std::vector<QueryDesc*>* qDescs;
} FaginsState;


TupleTableSlot* execFagins(PlanState* node){
	FaginsState* estate = (FaginsState*) node;
	uint32_t k = estate->topk;
	uint32_t ef = estate->ef;
	uint32_t term_cond = estate->term_cond;
	uint32_t consecutive_drops = 0;
	
	auto proc_pq = estate->proc_pq;
	auto seenSet = estate->seenSet;
	auto result_stack = estate->result_stack;
	bool isSingleCol = estate->children->size() == 1;

	// slot
	TupleTableSlot* result_slot = estate->ps.ps_ResultTupleSlot;
	
	while (!estate->finish){
		TupleTableSlot* slot;

		for ( unsigned int orderby_id = 0; orderby_id < estate->children->size(); orderby_id++){
			auto node	    = estate->children->at(orderby_id);
			auto q_estate   = estate->qDescs->at(orderby_id)->estate;
			auto oldcontext = MemoryContextSwitchTo(q_estate->es_query_cxt);

			slot = ExecProcNode((PlanState*) node);
					
			if ( TupIsNull(slot) ) {
				estate->finish = true;
				MemoryContextSwitchTo(oldcontext);
				break;
			}

			// attribute 1 is the result of rank expression by default			  
			float rank_score;
			if (estate->children->size() > 1){
			    rank_score= castAttrToFloat(slot, slot->tts_tupleDescriptor->natts-(isSingleCol? 0: 1));
			} else{
			    rank_score = DatumGetFloat4(node->iss_ScanDesc->xs_orderbyvals[0]);
			}

			// get tuple's tid, or row id
			ItemPointerData tid = node->iss_ScanDesc->xs_heaptid_orig;

			DEBUG_PRINT("Fagins Index Scan[%u]: heapRelation = %d, tid (%u, %u, %u), rank_score = %f ", orderby_id,
						node->ss.ss_currentRelation->rd_id,
						tid.ip_blkid.bi_hi, tid.ip_blkid.bi_lo, tid.ip_posid, rank_score);
		
			uint64_t tid_uint = castToUint64(tid);
			if ( seenSet->find(tid_uint) != seenSet->end() )
				continue;

			seenSet->insert(tid_uint);

			consecutive_drops++;
			if ( proc_pq->size() < ef || proc_pq->top().first > rank_score ){
				if ( proc_pq->size() == ef )
					proc_pq->pop();
				proc_pq->push(std::make_pair(rank_score, copyAndDropLastItem(slot)));
				consecutive_drops = 0;
			}

			MemoryContextSwitchTo(oldcontext);
		}

		// check termination cond
		if ( consecutive_drops >= term_cond ) {
			estate->finish = true;
		}
	}

	// put back the output into the correct order, i.e. smallest score first
	/*
	uint32_t i = ef - k;
	while (i > 0){
	        proc_pq->pop();
		i--;
	}
	*/

	while (!proc_pq->empty()) {
		result_stack->push(proc_pq->top());
		proc_pq->pop();
	}

	DEBUG_PRINT("Fagins Exec Node producing result, pending Results = %lu", result_stack->size());
	
	if (result_stack->size() == 0)
		return ExecClearTuple(result_slot);
	
	auto [rk_score, tuple] = result_stack->top();
	result_stack->pop();

	ExecForceStoreHeapTuple(tuple, result_slot, true);
	return result_slot;

}


FaginsState* InitFaginsState(Relation relation, uint32_t k, uint32_t ef, uint32_t term_cond, std::vector<IndexScanState*> scans, std::vector<QueryDesc*> qDescs){
	FaginsState* state = (FaginsState*)malloc(sizeof(FaginsState));
	memset(&(state->ps), 0, sizeof(PlanState));
	
	state->ps.ps_ResultTupleDesc = copyAndDropLastAttr(scans[0]->ss.ps.ps_ResultTupleDesc, scans.size()>1);
	state->ps.ps_ResultTupleSlot = MakeTupleTableSlot(state->ps.ps_ResultTupleDesc, scans[0]->ss.ps.ps_ResultTupleSlot->tts_ops);

	state->ps.ExecProcNode		 = execFagins;
	
	state->relation				 = relation;
	state->finish				 = false;
	state->topk					 = k;
	state->ef = ef;
	state->term_cond			  = term_cond;

	state->result_heap_scan		 = table_index_fetch_begin(relation);
	state->proc_pq               = new std::priority_queue<pq_item, std::vector<pq_item>, pq_item_compare_lt>();// proc_pq;
	state->seenSet               = new std::unordered_set<uint64_t>();// seenSet;

	state->result_stack			 = new std::stack<pq_item>();
	state->children				 = new std::vector<IndexScanState*>(scans);
	state->qDescs				 = new std::vector<QueryDesc*>(qDescs);
	
	return state;
}

void EndFaginsState(FaginsState *state){
	if (!state) return;
	
	table_index_fetch_end(state->result_heap_scan);
	table_close(state->relation, AccessShareLock);
	ExecDropSingleTupleTableSlot(state->ps.ps_ResultTupleSlot);

	for (QueryDesc* queryDesc: *(state->qDescs)) {
		ExecutorFinish(queryDesc);
		ExecutorEnd(queryDesc);
		FreeQueryDesc(queryDesc);
	}
	delete(state->proc_pq);
	delete(state->seenSet);
	delete(state->result_stack);
	free(state);
	free(state->qDescs);
	return;
}


Datum topk(PG_FUNCTION_ARGS) {

	FuncCallContext		*funcctx;
	int					 call_cntr;
	int					 max_calls;
	TupleDesc			 tupdesc;
	AttInMetadata		*attinmeta;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext	oldcontext;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		// SECTION BEGIN: start of multi-index topk initialization
		int argc = 0;
		text *tablename = PG_GETARG_TEXT_PP(argc++);
		Oid table_oid = getrelid(std::string(text_to_cstring(tablename)));

	        uint32 k = PG_GETARG_UINT32(argc++);

                uint32 ef = PG_GETARG_UINT32(argc++);
		if (ef == 0){
			ef = 64;
		}
		ef = std::max(ef, k);

		uint32 termCond = PG_GETARG_UINT32(argc++);
		if (termCond == 0)
			termCond = 50;
		DEBUG_PRINT("table = %s, table_oid = %u, k = %u", text_to_cstring(tablename), table_oid, k);

		text* attr_exp_text = PG_GETARG_TEXT_PP(argc++);

		text* filter_exp_text = PG_GETARG_TEXT_PP(argc++);
		DEBUG_PRINT("filter_exp = %s", text_to_cstring(filter_exp_text));
		text* rank_expression_text = PG_GETARG_TEXT_PP(argc++);
		// DEBUG_PRINT("Rank_exp = %s", text_to_cstring(rank_expression_text));
		auto orderby_expressions = convert_array_to_vector_str(PG_GETARG_DATUM(argc++));


		std::string rank_exp(text_to_cstring(rank_expression_text));
		// DEBUG_PRINT("Setting Rank_exp %lu", rank_exp.size());
		if ( rank_exp.size() == 0 ){
			int sz = orderby_expressions.size();
			for ( int i = 0; i < sz; i++ ) {
				std::string orderby = orderby_expressions[i];
				rank_exp+=("("+orderby+")");
				if ( i < sz-1 ) rank_exp+=" + ";
			}
		}

		DEBUG_PRINT("RANK Expresion = %s", rank_exp.c_str());
		
		std::vector<IndexScanState*> scannodes;
		std::vector<QueryDesc*> qDescs;
		SPI_connect();

		// prepare sql plans, each contain the corresponding index scan execution node for the orderby expression.
		for ( std::string exp : orderby_expressions) {
			DEBUG_PRINT("Prepare IndexScan node for orderby_expresion: %s ", exp.c_str());
		
			char sourceText[102400];

			if (orderby_expressions.size() == 1)
			{
				if ( strlen(text_to_cstring(filter_exp_text)) == 0){
					snprintf(sourceText, sizeof(sourceText), "select %s from %s order by %s",  text_to_cstring(attr_exp_text), text_to_cstring(tablename), exp.c_str());
				} else {
					snprintf(sourceText, sizeof(sourceText), "select %s from %s where %s order by %s", text_to_cstring(attr_exp_text), text_to_cstring(tablename), text_to_cstring(filter_exp_text), exp.c_str());
				}
			} else{
				if ( strlen(text_to_cstring(filter_exp_text)) == 0){
					snprintf(sourceText, sizeof(sourceText), "select %s, %s from %s order by %s",  text_to_cstring(attr_exp_text), rank_exp.c_str(), text_to_cstring(tablename), exp.c_str());
				} else {
					snprintf(sourceText, sizeof(sourceText), "select %s, %s from %s where %s order by %s", text_to_cstring(attr_exp_text), rank_exp.c_str(), text_to_cstring(tablename), text_to_cstring(filter_exp_text), exp.c_str());
				}
			}

			PlannedStmt* plan = NULL;
		
			if ( !(plan = extractIndexScanNode(sourceText)) ){
				elog(ERROR, "cannot find index for order by expr = %s", sourceText);
			}

			// DEBUG_PRINT("Create queryEnv");
			QueryEnvironment *queryEnv = create_queryEnv(); /* query environment setup for SPI level */
			DestReceiver * dest= CreateDestReceiver(DestNone);
			// DEBUG_PRINT("Create QueryDesc");
			QueryDesc* queryDesc = CreateQueryDesc(plan, sourceText,
												   GetActiveSnapshot(), InvalidSnapshot,
												   dest, NULL, queryEnv, 0);

			qDescs.push_back(queryDesc);
			// DEBUG_PRINT("Executor Start");
			standard_ExecutorStart(queryDesc, 0);
	
			// DEBUG_PRINT("Find IndexScanState");
			IndexScanState* node = findIndexScanState(queryDesc->planstate);

			// elog_node_display(WARNING, "INDEXSCAN", node, true);
			// DEBUG_PRINT("scan indexRelation (%p)", node->iss_ScanDesc->indexRelation);

			scannodes.push_back(node);
		}

		FaginsState* top_execNode = InitFaginsState(table_open(table_oid,AccessShareLock),
													k, ef, termCond, scannodes, qDescs);
	
		// SECTION END

		

		// SECTION START: following section sets up the composite return type

		/* store the multi-index topK in function call context */
		funcctx->user_fctx = top_execNode;
		
		/* total number of tuples to be returned */
		funcctx->max_calls = k;

		/* find a tuple descriptor for our result type: this is not required*/
		// tupdesc = top_execNode->ps.ps_ResultTupleSlot->tts_tupleDescriptor;
		tupdesc = top_execNode->ps.ps_ResultTupleDesc;

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(oldcontext);
		// SECTION END
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;
	
	FaginsState* top_execNode = (FaginsState*)funcctx->user_fctx;

	if (call_cntr < max_calls) {	/* do when there is more left to send */

		TupleTableSlot *slot;

		slot = ExecProcNode((PlanState*) top_execNode);
		if ( TupIsNull(slot) ) {
			goto finish;
		}

		Datum result = ExecFetchSlotHeapTupleDatum(slot);

		SRF_RETURN_NEXT(funcctx, result);
	}
	else{	 /* do when there is no more left */
		goto finish;
	}

 finish:

	EndFaginsState(top_execNode);

	SPI_finish();
	
	DEBUG_PRINT("Execute Fagins Multi-Column TopK Done, processed = %u", call_cntr);

	SRF_RETURN_DONE(funcctx);
			
}
