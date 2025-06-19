//
//  TWTimeFrames.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/base/TWTimeframes.h"
//
#include "nb/core/NBMngrStructMaps.h"

//TWTimeframe

STNBEnumMapRecord TWTimeframe_sharedEnumMapRecs[] = {
	{ ENTWTimeframe_Atom, "ENTWTimeframe_Atom", "atom" }
	, { ENTWTimeframe_Minute, "ENTWTimeframe_Minute", "min" }
	, { ENTWTimeframe_Hour, "ENTWTimeframe_Hour", "hour" }
	, { ENTWTimeframe_Day, "ENTWTimeframe_Day", "day" }
};

STNBEnumMap TWTimeframe_sharedEnumMap = {
	"ENTWTimeframe"
	, TWTimeframe_sharedEnumMapRecs
	, (sizeof(TWTimeframe_sharedEnumMapRecs) / sizeof(TWTimeframe_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TWTimeframe_getSharedEnumMap(void){
	return &TWTimeframe_sharedEnumMap;
}

//

const UI64 __dividers[] = {
	0, //ENTWTimeframe_Atom
	60, //ENTWTimeframe_Minute
	3600, //ENTWTimeframe_Hour
	86400, //ENTWTimeframe_Day
};

UI64 TWTimeframe_getDivider(const ENTWTimeframe timeframe, const UI64 atomicSecs){
	NBASSERT(ENTWTimeframe_Count == (sizeof(__dividers) / sizeof(__dividers[0])))
	if(timeframe == ENTWTimeframe_Atom){
		return atomicSecs;
	} else if(timeframe >= 0 && timeframe < ENTWTimeframe_Count){
		return __dividers[timeframe];
	}
	NBASSERT(FALSE)
	return 0;
}

UI64 TWTimeframe_getBaseTimestamp(const ENTWTimeframe timeframe, const UI64 atomicSecs, const UI64 timestamp){
	const UI64 div = TWTimeframe_getDivider(timeframe, atomicSecs);
	return (div == 0 ? timestamp : ((timestamp / div) * div));
}
