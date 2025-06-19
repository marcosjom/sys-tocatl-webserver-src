//
//  TWStatsTraffic.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#ifndef TWStatsTraffic_h
#define TWStatsTraffic_h

#include "nb/core/NBStructMap.h"
#include "nb/core/NBArray.h"
#include "core/base/TWTimeframes.h"

#ifdef __cplusplus
extern "C" {
#endif

//TWStatsTrafficFlow

typedef struct STTWStatsTrafficFlow_ {
	SI64	count;	//count (times read or write action)
	SI64	bytes;	//trafic
} STTWStatsTrafficFlow;
	
const STNBStructMap* TWStatsTrafficFlow_getSharedStructMap(void);

//TWStatsTrafficData

typedef struct STTWStatsTrafficData_ {
	SI64 alive;				//live clients (connected)
	SI64 accepted;			//accepted clients (incomming connections)
	STTWStatsTrafficFlow in;	//incoming traffic
	STTWStatsTrafficFlow out;	//outgoing traffic
} STTWStatsTrafficData;
	
const STNBStructMap* TWStatsTrafficData_getSharedStructMap(void);

void TWStatsTrafficData_resetExceptLive(STTWStatsTrafficData* obj);

//TWStatsTrafficFrame

typedef struct STTWStatsTrafficFrame_ {
	UI64				start;		//frame start UTC timestamp
	UI64				end;		//frame start UTC timestamp
	UI32				secsMissed;	//seconds inside frame that should not be included (gaps inside frames, ex: server-downtime)
	STTWStatsTrafficData tot;		//total values at hist-record
	STTWStatsTrafficData min;		//minimun values at hist-record
	STTWStatsTrafficData max;		//maximun values at hist-record
} STTWStatsTrafficFrame;
	
const STNBStructMap* TWStatsTrafficFrame_getSharedStructMap(void);

void TWStatsTrafficFrame_apply(STTWStatsTrafficFrame* obj, const STTWStatsTrafficData* accum, const UI64 time);

//TWStatsTrafficFrames

typedef struct STTWStatsTrafficFrames_ {
	STTWStatsTrafficFrame	frames[ENTWTimeframe_Count];	//active values per timeframe (atomic, minute, hour, day)
	STTWStatsTrafficData	pend;	//data pending to apply
	//hist
	struct {
		STNBArray		arrs[ENTWTimeframe_Count];	//STTWStatsTrafficFrame, history arrays per timeframe
		UI32			maxs[ENTWTimeframe_Count];	//max history size
	} hist;
} STTWStatsTrafficFrames;
	
void TWStatsTrafficFrames_init(STTWStatsTrafficFrames* obj);
void TWStatsTrafficFrames_release(STTWStatsTrafficFrames* obj);
//
void TWStatsTrafficFrames_setMaxHistAmount(STTWStatsTrafficFrames* obj, const ENTWTimeframe timeframe, const UI32 amm);
void TWStatsTrafficFrames_timeframeChanged(STTWStatsTrafficFrames* obj, const ENTWTimeframe timeframe, const UI64 time);

//TWStatsTraffic

typedef struct STTWStatsTraffic_ {
	STTWStatsTrafficFrames		total;
	struct STTWStatsTraffic_*	prnt;
} STTWStatsTraffic;

void TWStatsTraffic_init(STTWStatsTraffic* obj);
void TWStatsTraffic_release(STTWStatsTraffic* obj);

//cfg
void TWStatsTraffic_setParentStats(STTWStatsTraffic* obj, STTWStatsTraffic* prnt);
void TWStatsTraffic_setMaxHistAmount(STTWStatsTraffic* obj, const ENTWTimeframe timeframe, const UI32 amm);

//actions
void TWStatsTraffic_connStarted(STTWStatsTraffic* obj);	//connection started
void TWStatsTraffic_connEnded(STTWStatsTraffic* obj);	//connection ended
void TWStatsTraffic_accepted(STTWStatsTraffic* obj);	//accepted client
void TWStatsTraffic_bytesRead(STTWStatsTraffic* obj, const SI64 bytes); 	//bytes received
void TWStatsTraffic_bytesWritten(STTWStatsTraffic* obj, const SI64 bytes);	//bytes sent

//apply
void TWStatsTraffic_consumePendActions(STTWStatsTraffic* obj, const UI64 time);
void TWStatsTraffic_timeframeChanged(STTWStatsTraffic* obj, const ENTWTimeframe timeframe, const UI64 time);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWStatsTraffic_h */
