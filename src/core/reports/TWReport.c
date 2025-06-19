//
//  TWReport.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/reports/TWReport.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//----------------------
//- STATS
//----------------------

//TWReportTrafficHistRec

STNBStructMapsRec TWReportTrafficHistRec_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWReportTrafficHistRec_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWReportTrafficHistRec_sharedStructMap);
	if(TWReportTrafficHistRec_sharedStructMap.map == NULL){
		STTWReportTrafficHistRec s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWReportTrafficHistRec);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addStructPtrM(map, s, all, TWStatsTrafficFrame_getSharedStructMap());		//all traffic
		NBStructMap_addStructPtrM(map, s, sources, TWStatsTrafficFrame_getSharedStructMap());	//sources (outgoind conns to old-api)
		NBStructMap_addStructPtrM(map, s, api, TWStatsTrafficFrame_getSharedStructMap());		//api (incoming conns to new-api)		
		//
		TWReportTrafficHistRec_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWReportTrafficHistRec_sharedStructMap);
	return TWReportTrafficHistRec_sharedStructMap.map;
}

//TWReportTrafficHist

STNBStructMapsRec TWReportTrafficHist_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWReportTrafficHist_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWReportTrafficHist_sharedStructMap);
	if(TWReportTrafficHist_sharedStructMap.map == NULL){
		STTWReportTrafficHist s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWReportTrafficHist);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addEnumM(map, s, timeframe, TWTimeframe_getSharedEnumMap()); //type of timeframe (atomic, minute, hour, day)
		NBStructMap_addPtrToArrayOfStructM(map, s, records, recordsSz, ENNBStructMapSign_Unsigned, TWReportTrafficHistRec_getSharedStructMap()); //records		
		//
		TWReportTrafficHist_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWReportTrafficHist_sharedStructMap);
	return TWReportTrafficHist_sharedStructMap.map;
}

//TWReportStats

STNBStructMapsRec TWReportStats_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWReportStats_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWReportStats_sharedStructMap);
	if(TWReportStats_sharedStructMap.map == NULL){
		STTWReportStats s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWReportStats);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addPtrToArrayOfStructM(map, s, traffic, trafficSz, ENNBStructMapSign_Unsigned, TWReportTrafficHist_getSharedStructMap()); //traffic report ('/stats')		
		//
		TWReportStats_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWReportStats_sharedStructMap);
	return TWReportStats_sharedStructMap.map;
}

//----------------------
//- REPORT
//----------------------

//TWReport

STNBStructMapsRec TWReport_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWReport_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWReport_sharedStructMap);
	if(TWReport_sharedStructMap.map == NULL){
		STTWReport s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWReport);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addUIntM(map, s, time);	//UTC-timestamp of report's content
		NBStructMap_addStructPtrM(map, s, stats, TWReportStats_getSharedStructMap()); //"/stats" report
		//
		TWReport_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWReport_sharedStructMap);
	return TWReport_sharedStructMap.map;
}
