//
//  TWCfgStats.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 1/12/21.
//

#ifndef TWCfgStats_h
#define TWCfgStats_h

#include "nb/core/NBStructMap.h"
#include "core/base/TWTimeframes.h"

#ifdef __cplusplus
extern "C" {
#endif

//TWCfgStats

typedef struct STTWCfgStatsHist_ {
	ENTWTimeframe		frame;		//frameId
	SI32				amount;		//TWmout of records to keep
} STTWCfgStatsHist;
	
const STNBStructMap* TWCfgStatsHist_getSharedStructMap(void);

typedef struct STTWCfgStats_ {
	SI32				atom;		//secs-per-tick (1-30 secs, defines the periddicity of events, messages and atats)
	STTWCfgStatsHist*	hist;		//TWount of history records per timeframe
	UI32				histSz;		//TWount of history records per timeframe
} STTWCfgStats;
	
const STNBStructMap* TWCfgStats_getSharedStructMap(void);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWCfgStats_h */
