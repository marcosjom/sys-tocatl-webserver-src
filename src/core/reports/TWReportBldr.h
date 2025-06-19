//
//  TWReportBldr.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#ifndef TWReportBldr_h
#define TWReportBldr_h

#include "nb/NBFrameworkDefs.h"
#include "core/reports/TWReport.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArraySorted.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------
//- STATS
//----------------------

//TWReportBldrStatsTraffic

typedef struct STTWReportBldrStatsTraffic_ {		
	STTWReportTrafficHist hist[ENTWTimeframe_Count];
	STNBArray			records[ENTWTimeframe_Count];	//STTWReportTrafficHistRec, static records to be added to report
} STTWReportBldrStatsTraffic;

void TWReportBldrStatsTraffic_init(STTWReportBldrStatsTraffic* obj);
void TWReportBldrStatsTraffic_release(STTWReportBldrStatsTraffic* obj);
void TWReportBldrStatsTraffic_setHist(STTWReportBldrStatsTraffic* obj, const ENTWTimeframe timeframe, STTWStatsTrafficFrame* all, STTWStatsTrafficFrame* sources, STTWStatsTrafficFrame* api, const UI32 histsSz, STTWReportTrafficHist** dstTraffic, UI32* dstTrafficSz);

//----------------------
//- BUILDER
//----------------------

//TWReportBldr

typedef struct STTWReportBldr_ {
	STTWReport		report;			//template
	//alloc
	struct {
		//stats
		struct {
			STTWReportStats record;	//static record to be inlined to report
			STTWReportBldrStatsTraffic traffic;
		} stats;
	} alloc;
} STTWReportBldr;

void TWReportBldr_init(STTWReportBldr* obj);
void TWReportBldr_release(STTWReportBldr* obj);

//

void TWReportBldr_start(STTWReportBldr* obj, const UI64 time);
void TWReportBldr_setStatsTrafficHist(STTWReportBldr* obj, const ENTWTimeframe timeframe, STTWStatsTrafficFrame* all, STTWStatsTrafficFrame* sources, STTWStatsTrafficFrame* api, const UI32 histsSz);

//

void TWReportBldr_buildReportJson(STTWReportBldr* obj, STNBString* dst);
void TWReportBldr_buildReportJsonWithFormat(STTWReportBldr* obj, STNBString* dst, const STNBStructConcatFormat* format);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWReportBldr_h */
