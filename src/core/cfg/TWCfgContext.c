//
//  TWCfgContext.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/cfg/TWCfgContext.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//TWCfgContext

STNBStructMapsRec TWCfgContext_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgContext_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWCfgContext_sharedStructMap);
	if(TWCfgContext_sharedStructMap.map == NULL){
		STTWCfgContext s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgContext);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addStructM(map, s, threads, NBThreadsPoolCfg_getSharedStructMap()); //threads-pool
        NBStructMap_addStructM(map, s, pollsters, NBIOPollstersPoolCfg_getSharedStructMap()); //pollsters
		//
		TWCfgContext_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWCfgContext_sharedStructMap);
	return TWCfgContext_sharedStructMap.map;
}
