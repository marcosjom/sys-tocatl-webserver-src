//
//  TWTimeFrames.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#ifndef TWTimeFrames_h
#define TWTimeFrames_h

#ifdef __cplusplus
extern "C" {
#endif

#include "nb/core/NBStructMap.h"

//ENTWTimeframe

typedef enum ENTWTimeframe_ {
	ENTWTimeframe_Atom = 0,		//1-30 seconds, defined in config-file
	ENTWTimeframe_Minute,		//(timestamp / 60 * 60)
	ENTWTimeframe_Hour,			//(timestamp / (3600) * 3600)
	ENTWTimeframe_Day,			//(timestamp / (86400) * 86400)
	//
	ENTWTimeframe_Count
} ENTWTimeframe;

const STNBEnumMap* TWTimeframe_getSharedEnumMap(void);

UI64 TWTimeframe_getDivider(const ENTWTimeframe timeframe, const UI64 atomicSecs);
UI64 TWTimeframe_getBaseTimestamp(const ENTWTimeframe timeframe, const UI64 atomicSecs, const UI64 timestamp);

#ifdef __cplusplus
} //extern "C"
#endif


#endif /* TWTimeFrames_h */
