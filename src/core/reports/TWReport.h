//
//  TWReport.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 27/11/21.
//

#ifndef TWReport_h
#define TWReport_h

#include "nb/NBFrameworkDefs.h"
#include "core/stats/TWStatsTraffic.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------
//- STATS
//----------------------

//TWReportTrafficHistRec

typedef struct STTWReportTrafficHistRec_ {
	STTWStatsTrafficFrame*	all;		//all traffic
	STTWStatsTrafficFrame*	sources;	//sources (outgoind conns to old-api)
	STTWStatsTrafficFrame*	api;		//api (incoming conns to new-api)
} STTWReportTrafficHistRec;

const STNBStructMap* TWReportTrafficHistRec_getSharedStructMap(void);

//TWReportTrafficHist

typedef struct STTWReportTrafficHist_ {
	ENTWTimeframe			timeframe;	//type of timeframe (atomic, minute, hour, day)
	STTWReportTrafficHistRec* records;	//records
	UI32					recordsSz;	//records
} STTWReportTrafficHist;

const STNBStructMap* TWReportTrafficHist_getSharedStructMap(void);

//TWReportStats

typedef struct STTWReportStats_ {
	STTWReportTrafficHist*	traffic;	//traffic report ('/stats')
	UI32					trafficSz;	//traffic report ('/stats')
} STTWReportStats;

const STNBStructMap* TWReportStats_getSharedStructMap(void);


//----------------------
//- REPORT
//----------------------

//TWReport

typedef struct STTWReport_ {
	UI64				time;		//UTC-timestamp of report's content
	STTWReportStats*	stats;		//"/stats" report
} STTWReport;

const STNBStructMap* TWReport_getSharedStructMap(void);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWReport_h */
