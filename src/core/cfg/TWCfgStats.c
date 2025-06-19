//
//  TWCfgStats.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 1/12/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/cfg/TWCfgStats.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//TWCfgStatsHist

STNBStructMapsRec TWCfgStatsHist_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgStatsHist_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWCfgStatsHist_sharedStructMap);
	if(TWCfgStatsHist_sharedStructMap.map == NULL){
		STTWCfgStatsHist s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgStatsHist);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addEnumM(map, s, frame, TWTimeframe_getSharedEnumMap()); //frameId
		NBStructMap_addIntM(map, s, amount); //TWmout of records to keep
		//
		TWCfgStatsHist_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWCfgStatsHist_sharedStructMap);
	return TWCfgStatsHist_sharedStructMap.map;
}

//TWCfgStats

STNBStructMapsRec TWCfgStats_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgStats_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWCfgStats_sharedStructMap);
	if(TWCfgStats_sharedStructMap.map == NULL){
		STTWCfgStats s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgStats);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addIntM(map, s, atom);				//secs-per-tick (1-30 secs, defines the periddicity of events, messages and atats)
		NBStructMap_addPtrToArrayOfStructM(map, s, hist, histSz, ENNBStructMapSign_Unsigned, TWCfgStatsHist_getSharedStructMap()); //TWount of history records per timeframe
		//
		TWCfgStats_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWCfgStats_sharedStructMap);
	return TWCfgStats_sharedStructMap.map;
}

