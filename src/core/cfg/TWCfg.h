//
//  TWCfg.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#ifndef TWCfg_h
#define TWCfg_h

#include "nb/core/NBStructMap.h"
#include "core/cfg/TWCfgStats.h"
#include "core/cfg/TWCfgWeb.h"
//
#include "core/cfg/TWCfgContext.h"
#include "nb/net/NBHttpService.h"

#ifdef __cplusplus
extern "C" {
#endif
	
//TWCfg

typedef struct STTWCfg_ {
    STTWCfgContext      context;    //context
	STTWCfgStats		stats;		//stats
    STNBHttpServiceCfg  http;       //http-server
    STTWCfgWeb          web;        //web-server
} STTWCfg;
	
const STNBStructMap* TWCfg_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWCfg_h */
