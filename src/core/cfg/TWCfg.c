//
//  TWCfg.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/cfg/TWCfg.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//TWCfg

STNBStructMapsRec TWCfg_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfg_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWCfg_sharedStructMap);
	if(TWCfg_sharedStructMap.map == NULL){
		STTWCfg s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfg);
		NBStructMap_init(map, sizeof(s));
		//members
		NBStructMap_addStructM(map, s, context, TWCfgContext_getSharedStructMap()); //context
		NBStructMap_addStructM(map, s, stats, TWCfgStats_getSharedStructMap());		//stats
		NBStructMap_addStructM(map, s, http, NBHttpServiceCfg_getSharedStructMap()); //http-server
        NBStructMap_addStructM(map, s, web, TWCfgWeb_getSharedStructMap());			//web-server
		//
		TWCfg_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWCfg_sharedStructMap);
	return TWCfg_sharedStructMap.map;
}
