//
//  TWStatsTrafficFlow.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/stats/TWStatsTraffic.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//TWStatsTrafficFlow

STNBStructMapsRec TWStatsTrafficFlow_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWStatsTrafficFlow_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWStatsTrafficFlow_sharedStructMap);
	if(TWStatsTrafficFlow_sharedStructMap.map == NULL){
		STTWStatsTrafficFlow s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWStatsTrafficFlow);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addIntM(map, s, count);	//count (times read or write action)
		NBStructMap_addIntM(map, s, bytes);	//trafic
		//
		TWStatsTrafficFlow_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWStatsTrafficFlow_sharedStructMap);
	return TWStatsTrafficFlow_sharedStructMap.map;
}

//TWStatsTraffic

STNBStructMapsRec TWStatsTrafficData_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWStatsTrafficData_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWStatsTrafficData_sharedStructMap);
	if(TWStatsTrafficData_sharedStructMap.map == NULL){
		STTWStatsTrafficData s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWStatsTrafficData);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addIntM(map, s, alive); //live clients (connected)
		NBStructMap_addIntM(map, s, accepted); //accepted clients
		NBStructMap_addStructM(map, s, in, TWStatsTrafficFlow_getSharedStructMap());	//incoming traffic
		NBStructMap_addStructM(map, s, out, TWStatsTrafficFlow_getSharedStructMap());	//outgoing traffic
		//
		TWStatsTrafficData_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWStatsTrafficData_sharedStructMap);
	return TWStatsTrafficData_sharedStructMap.map;
}

void TWStatsTrafficData_resetExceptLive(STTWStatsTrafficData* obj){
	//obj->alive = 0; //kept
	obj->accepted = 0;
	//
	NBStruct_stRelease(TWStatsTrafficFlow_getSharedStructMap(), &obj->in, sizeof(obj->in));
	NBStruct_stRelease(TWStatsTrafficFlow_getSharedStructMap(), &obj->out, sizeof(obj->out));
}

//TWStatsTrafficFrame

STNBStructMapsRec TWStatsTrafficFrame_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWStatsTrafficFrame_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWStatsTrafficFrame_sharedStructMap);
		if(TWStatsTrafficFrame_sharedStructMap.map == NULL){
			STTWStatsTrafficFrame s;
			STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWStatsTrafficFrame);
			NBStructMap_init(map, sizeof(s));
			//members
			NBStructMap_addUIntM(map, s, start);		//frame start UTC timestamp
			NBStructMap_addUIntM(map, s, end);			//frame start UTC timestamp
			NBStructMap_addUIntM(map, s, secsMissed);	//seconds inside frame that should not be included (gaps inside frames, ex: server-downtime)
			NBStructMap_addStructM(map, s, tot, TWStatsTrafficData_getSharedStructMap());
			NBStructMap_addStructM(map, s, min, TWStatsTrafficData_getSharedStructMap());
			NBStructMap_addStructM(map, s, max, TWStatsTrafficData_getSharedStructMap());
			//
			TWStatsTrafficFrame_sharedStructMap.map = map;
		}
		NBMngrStructMaps_unlock(&TWStatsTrafficFrame_sharedStructMap);
		return TWStatsTrafficFrame_sharedStructMap.map;
}

void TWStatsTrafficFrame_apply(STTWStatsTrafficFrame* obj, const STTWStatsTrafficData* accum, const UI64 time){
	obj->end = time;
	//apply min
	{
		if(obj->min.accepted > accum->accepted) { obj->min.accepted = accum->accepted; };
		if(obj->min.alive > accum->alive) { obj->min.alive = accum->alive; };
		if(obj->min.in.count > accum->in.count) { obj->min.in.count = accum->in.count; };
		if(obj->min.in.bytes > accum->in.bytes) { obj->min.in.bytes = accum->in.bytes; };
		if(obj->min.out.count > accum->out.count) { obj->min.out.count = accum->out.count; };
		if(obj->min.out.bytes > accum->out.bytes) { obj->min.out.bytes = accum->out.bytes; };
	}
	//apply max
	{
		if(obj->max.accepted < accum->accepted) { obj->max.accepted = accum->accepted; };
		if(obj->max.alive < accum->alive) { obj->max.alive = accum->alive; };
		if(obj->max.in.count < accum->in.count) { obj->max.in.count = accum->in.count; };
		if(obj->max.in.bytes < accum->in.bytes) { obj->max.in.bytes = accum->in.bytes; };
		if(obj->max.out.count < accum->out.count) { obj->max.out.count = accum->out.count; };
		if(obj->max.out.bytes < accum->out.bytes) { obj->max.out.bytes = accum->out.bytes; };
	}
	//apply to total
	{
		obj->tot.accepted += accum->accepted;
		//obj->tot.alive += accum->alive; //do not apply 'live' to tatol
		obj->tot.in.count += accum->in.count;
		obj->tot.in.bytes += accum->in.bytes;
		obj->tot.out.count += accum->out.count;
		obj->tot.out.bytes += accum->out.bytes;
	}
}

//TWStatsTrafficFrames

void TWStatsTrafficFrames_init(STTWStatsTrafficFrames* obj){
	NBMemory_setZeroSt(*obj, STTWStatsTrafficFrames);
	//set start times
	{
		SI32 i; for(i = 0; i < (sizeof(obj->frames) / sizeof(obj->frames[0])); i++){
			STTWStatsTrafficFrame* f = &obj->frames[i];
			f->start = f->end = NBDatetime_getCurUTCTimestamp();
		}
	}
	//hist
	{
		//arrays
		{
			SI32 i; for(i = 0; i < (sizeof(obj->hist.arrs) / sizeof(obj->hist.arrs[0])); i++){
				STNBArray* a = &obj->hist.arrs[i];
				NBArray_init(a, sizeof(STTWStatsTrafficFrame), NULL);
			}
		}
	}
}

void TWStatsTrafficFrames_release(STTWStatsTrafficFrames* obj){
	const STNBStructMap* hMap = TWStatsTrafficFrame_getSharedStructMap();
	const STNBStructMap* dMap = TWStatsTrafficData_getSharedStructMap();
	//frames
	{
		SI32 i; for(i = 0; i < (sizeof(obj->frames) / sizeof(obj->frames[0])); i++){
			NBStruct_stRelease(hMap, &obj->frames[i], sizeof(obj->frames[i]));
		}
	}
	//pend
	{
		NBStruct_stRelease(dMap, &obj->pend, sizeof(obj->pend));
	}
	//hist
	{
		// arrays
		{
			SI32 i; for(i = 0; i < (sizeof(obj->hist.arrs) / sizeof(obj->hist.arrs[0])); i++){
				STNBArray* a = &obj->hist.arrs[i];
				SI32 i2; for(i2 = 0; i2 < a->use; i2++){
					STTWStatsTrafficFrame* f = NBArray_itmPtrAtIndex(a, STTWStatsTrafficFrame, i2);
					NBStruct_stRelease(hMap, f, sizeof(*f));
				}
				NBArray_empty(a);
				NBArray_release(a);
			}
		}
	}
}

void TWStatsTrafficFrames_setMaxHistAmount(STTWStatsTrafficFrames* obj, const ENTWTimeframe timeframe, const UI32 amm){
	if(timeframe >= 0 && timeframe < ENTWTimeframe_Count){
		obj->hist.maxs[timeframe] = amm;
	}
}

void TWStatsTrafficFrames_timeframeChanged(STTWStatsTrafficFrames* obj, const ENTWTimeframe timeframe, const UI64 time){
	if(timeframe >= 0 && timeframe < ENTWTimeframe_Count){
		STTWStatsTrafficFrame* f = &obj->frames[timeframe];
		STNBArray* a = &obj->hist.arrs[timeframe];
		NBArray_addValue(a, *f);
		//release extras but last
		while(a->use > 1 && a->use > obj->hist.maxs[timeframe]){
			STTWStatsTrafficFrame* old = NBArray_itmPtrAtIndex(a, STTWStatsTrafficFrame, 0);
			NBStruct_stRelease(TWStatsTrafficFrame_getSharedStructMap(), old, sizeof(*old));
			NBArray_removeItemAtIndex(a, 0);
		}
		//reset new frame
		{
			NBStruct_stRelease(TWStatsTrafficFrame_getSharedStructMap(), f, sizeof(*f));
			f->start = f->end = time;
		}
	}
}

//TWStatsTraffic

/*typedef struct STTWStatsTraffic_ {
	STTWStatsTrafficData pair;
	struct STTWStatsTraffic_* prnt;
} STTWStatsTraffic;*/

void TWStatsTraffic_init(STTWStatsTraffic* obj){
	NBMemory_setZeroSt(*obj, STTWStatsTraffic);
	TWStatsTrafficFrames_init(&obj->total);
}

void TWStatsTraffic_release(STTWStatsTraffic* obj){
	//total
	{
		TWStatsTrafficFrames_release(&obj->total);
	}
	//prnt
	{
		obj->prnt = NULL;
	}
}

//cfg

void TWStatsTraffic_setParentStats(STTWStatsTraffic* obj, STTWStatsTraffic* prnt){
	obj->prnt = prnt;
}

void TWStatsTraffic_setMaxHistAmount(STTWStatsTraffic* obj, const ENTWTimeframe timeframe, const UI32 amm){
	TWStatsTrafficFrames_setMaxHistAmount(&obj->total, timeframe, amm);
}

//actions

void TWStatsTraffic_connStarted(STTWStatsTraffic* obj){	//connection started
	STTWStatsTrafficData* pend = &obj->total.pend;
	pend->alive++;
	if(obj->prnt != NULL){
		TWStatsTraffic_connStarted(obj->prnt);
	}
}

void TWStatsTraffic_connEnded(STTWStatsTraffic* obj){	//connection ended
	STTWStatsTrafficData* pend = &obj->total.pend;
	NBASSERT(pend->alive > 0) //program logic error
	if(pend->alive > 0){
		pend->alive--;
	}
	if(obj->prnt != NULL){
		TWStatsTraffic_connEnded(obj->prnt);
	}
}

void TWStatsTraffic_accepted(STTWStatsTraffic* obj){	//accepted client
	STTWStatsTrafficData* pend = &obj->total.pend;
	pend->accepted++;
	if(obj->prnt != NULL){
		TWStatsTraffic_accepted(obj->prnt);
	}
}

void TWStatsTraffic_bytesRead(STTWStatsTraffic* obj, const SI64 bytes){
	STTWStatsTrafficData* pend = &obj->total.pend;
	pend->in.count++;
	pend->in.bytes += bytes;
	if(obj->prnt != NULL){
		TWStatsTraffic_bytesRead(obj->prnt, bytes);
	}
}

void TWStatsTraffic_bytesWritten(STTWStatsTraffic* obj, const SI64 bytes){
	STTWStatsTrafficData* pend = &obj->total.pend;
	pend->out.count++;
	pend->out.bytes += bytes;
	if(obj->prnt != NULL){
		TWStatsTraffic_bytesWritten(obj->prnt, bytes);
	}
}

//apply

void TWStatsTraffic_consumePendActions(STTWStatsTraffic* obj, const UI64 time){
	//total
	{
		STTWStatsTrafficFrames* pair = &obj->total;
		{
			SI32 i2; for(i2 = 0; i2 < (sizeof(pair->frames) / sizeof(pair->frames[0])); i2++){
				TWStatsTrafficFrame_apply(&pair->frames[i2], &pair->pend, time);
			}
		}
		TWStatsTrafficData_resetExceptLive(&pair->pend);
	}
}

void TWStatsTraffic_timeframeChanged(STTWStatsTraffic* obj, const ENTWTimeframe timeframe, const UI64 time){
	TWStatsTrafficFrames_timeframeChanged(&obj->total, timeframe, time);
}
