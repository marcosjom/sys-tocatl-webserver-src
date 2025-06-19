//
//  TWReportBldr.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/reports/TWReportBldr.h"
//
#include "nb/core/NBMemory.h"

//----------------------
//- STATS
//----------------------

//TWReportBldrStatsTraffic

void TWReportBldrStatsTraffic_init(STTWReportBldrStatsTraffic* obj){
	SI32 i; for(i = 0; i < (sizeof(obj->records) / sizeof(obj->records[0])); i++){
		NBArray_init(&obj->records[i], sizeof(STTWReportTrafficHistRec), NULL);
	}
}

void TWReportBldrStatsTraffic_release(STTWReportBldrStatsTraffic* obj){
	SI32 i; for(i = 0; i < (sizeof(obj->records) / sizeof(obj->records[0])); i++){
		STNBArray* arr = &obj->records[i];
		NBArray_empty(arr);
		NBArray_release(arr);
	}
}

void TWReportBldrStatsTraffic_setHist(STTWReportBldrStatsTraffic* obj, const ENTWTimeframe timeframe, STTWStatsTrafficFrame* all, STTWStatsTrafficFrame* sources, STTWStatsTrafficFrame* api, const UI32 histsSz, STTWReportTrafficHist** dstTraffic, UI32* dstTrafficSz){
	if(timeframe >= 0 && timeframe < ENTWTimeframe_Count && dstTraffic != NULL && dstTrafficSz != NULL){
		STTWReportTrafficHist* rec = NULL;
		STNBArray* arr = NULL; 
		//search (in case added multiple times)
		if(*dstTraffic != NULL && *dstTrafficSz > 0){
			UI32 i; for(i = 0; i < *dstTrafficSz; i++){
				if((*dstTraffic)[i].timeframe == timeframe){
					rec = &(*dstTraffic)[i];
					arr = &obj->records[i];
					break;
				}
			}
		}
		//add record
		if(rec == NULL){
			if(*dstTraffic == NULL || *dstTrafficSz <= 0){
				*dstTraffic		= obj->hist;
				*dstTrafficSz	= 0;
			}
			rec = &(*dstTraffic)[*dstTrafficSz];
			arr = &obj->records[*dstTrafficSz];
			*dstTrafficSz = *dstTrafficSz + 1;
		}
		//populate
		rec->timeframe	= timeframe;
		{
			NBArray_empty(arr);
			{
				STTWReportTrafficHistRec rr;
				UI32 i; for(i = 0; i < histsSz; i++){
					NBMemory_setZeroSt(rr, STTWReportTrafficHistRec);
					rr.all		= (all != NULL ? &all[i] : NULL);
					rr.sources	= (sources != NULL ? &sources[i] : NULL);
					rr.api		= (api != NULL ? &api[i] : NULL);
					NBArray_addValue(arr, rr);
				}
			}
			rec->records	= NBArray_dataPtr(arr, STTWReportTrafficHistRec);
			rec->recordsSz	= arr->use;
		}
	}
}

//----------------------
//- BUILDER
//----------------------

//TWReportBldr

void TWReportBldr_init(STTWReportBldr* obj){
	NBMemory_setZeroSt(*obj, STTWReportBldr);
	//alloc
	{
		//stats
		{
			TWReportBldrStatsTraffic_init(&obj->alloc.stats.traffic);
		}
	}
}

void TWReportBldr_release(STTWReportBldr* obj){
	//alloc
	{
		//stats
		{
			TWReportBldrStatsTraffic_release(&obj->alloc.stats.traffic);
		}
	}
}

//

void TWReportBldr_start(STTWReportBldr* obj, const UI64 time){
	//alloc
	{
		//traffic
		{
			SI32 i; for(i = 0; i < (sizeof(obj->alloc.stats.traffic.records) / sizeof(obj->alloc.stats.traffic.records[0])); i++){
				NBArray_empty(&obj->alloc.stats.traffic.records[i]);
			}
		}
	}
	//init
	NBMemory_setZeroSt(obj->report, STTWReport);
	obj->report.time = time;
}

void TWReportBldr_setStatsTrafficHist(STTWReportBldr* obj, const ENTWTimeframe timeframe, STTWStatsTrafficFrame* all, STTWStatsTrafficFrame* sources, STTWStatsTrafficFrame* api, const UI32 histsSz){
	STTWReportStats* stats = &obj->alloc.stats.record;
	if(obj->report.stats == NULL){
		NBMemory_setZeroSt(obj->alloc.stats.record, STTWReportStats);
		obj->report.stats = stats;
	}
	TWReportBldrStatsTraffic_setHist(&obj->alloc.stats.traffic, timeframe, all, sources, api, histsSz, &stats->traffic, &stats->trafficSz);
}

//

void TWReportBldr_buildReportJson(STTWReportBldr* obj, STNBString* dst){
	if(dst != NULL){
		NBStruct_stConcatAsJson(dst, TWReport_getSharedStructMap(), &obj->report, sizeof(obj->report));
	}
}

void TWReportBldr_buildReportJsonWithFormat(STTWReportBldr* obj, STNBString* dst, const STNBStructConcatFormat* format){
	if(dst != NULL){
		NBStruct_stConcatAsJsonWithFormat(dst, TWReport_getSharedStructMap(), &obj->report, sizeof(obj->report), format);
	}
}
