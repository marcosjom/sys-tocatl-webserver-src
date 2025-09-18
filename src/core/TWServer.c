//
//  TWServer.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/TWServer.h"
#include "core/base/TWMimeTypesDefaults.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBStopFlag.h"
#include "nb/core/NBNumParser.h"
#include "nb/core/NBEncoding.h"
#include "nb/net/NBHttpService.h"
#include "nb/net/NBHttpServiceRespLnk.h"

//TWServerStatsFrame

typedef struct STTWServerStatsFrame_ {
    UI64 frameStart;        //timestamp mod
    STNBHttpStatsData data;
} STTWServerStatsFrame;

void TWServerStatsFrame_init(STTWServerStatsFrame* obj);
void TWServerStatsFrame_release(STTWServerStatsFrame* obj);

//TWServerOpq

typedef struct STTWServerOpq_ {
    STNBObject          prnt;
    STNBThreadCond      cond;
    STNBStopFlagRef     stopFlag;
    STTWCfg             cfg;    //config
    STNBHttpServiceRef  srv;    //service
    STNBFilesystem      fs;     //filesystem
    //stats
    struct {
        STTWServerStatsFrame dataPerFrame[ENTWTimeframe_Count];
    } stats;
} STTWServerOpq;

NB_OBJREF_BODY(TWServer, STTWServerOpq, NBObject);

//Http

BOOL TWServer_httpCltConnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* usrData); //new client connected
void TWServer_httpCltDisconnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* usrData); //connection was closed or lost
BOOL TWServer_httpCltReqArrived_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk, void* usrData);    //called when header-frist-line arrived, when header completion arrived or when body completing arrived; first to populate required methods into 'dstLtnr' take ownership and stops further calls to this method.

//TWServer

void TWServer_initZeroed(STNBObject* obj) {
    STTWServerOpq* opq = (STTWServerOpq*)obj;
    NBThreadCond_init(&opq->cond);
    opq->stopFlag = NBStopFlag_alloc(NULL);
    //cfg
    {
        //
    }
    //service
    {
        //
    }
    //fs
    {
        NBFilesystem_init(&opq->fs);
    }
    //stats
    {
        SI32 i; for (i = 0; i < (sizeof(opq->stats.dataPerFrame) / sizeof(opq->stats.dataPerFrame[0])); i++) {
            STTWServerStatsFrame* f = &opq->stats.dataPerFrame[i];
            TWServerStatsFrame_init(f);
        }
    }
}

void TWServer_uninitLocked(STNBObject* obj){
    STTWServerOpq* opq = (STTWServerOpq*)obj;
    //
    NBStopFlag_activate(opq->stopFlag);
    //cfg
    {
        NBStruct_stRelease(TWCfg_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
    }
    //service
    {
        if(NBHttpService_isSet(opq->srv)){
            NBHttpService_release(&opq->srv);
            NBHttpService_null(&opq->srv);
        }
    }
    //stats
    {
        SI32 i; for (i = 0; i < (sizeof(opq->stats.dataPerFrame) / sizeof(opq->stats.dataPerFrame[0])); i++) {
            STTWServerStatsFrame* f = &opq->stats.dataPerFrame[i];
            TWServerStatsFrame_release(f);
        }
    }
    //fs
    {
        NBFilesystem_release(&opq->fs);
    }
    if(NBStopFlag_isSet(opq->stopFlag)){
        NBStopFlag_release(&opq->stopFlag);
        NBStopFlag_null(&opq->stopFlag);
    }
    opq->stopFlag = NBStopFlag_alloc(NULL);
    NBThreadCond_release(&opq->cond);
}

//

void TWServer_stopFlag(STTWServerRef ref){ //stop-flag method
    STTWServerOpq* opq = (STTWServerOpq*)ref.opaque; NBASSERT(TWServer_isClass(ref))
    NBStopFlag_activate(opq->stopFlag);
}

void TWServer_waitForAll(STTWServerRef ref){ //stop
    STTWServerOpq* opq = (STTWServerOpq*)ref.opaque; NBASSERT(TWServer_isClass(ref))
    NBObject_lock(opq);
    while(NBHttpService_isSet(opq->srv) && NBHttpService_isBusy(opq->srv)){
        NBObject_unlock(opq);
        {
            NBThread_mSleep(100);
        }
        NBObject_lock(opq);
    }
    NBObject_unlock(opq);
}


BOOL TWServer_prepare(STTWServerRef ref, const STTWCfg* cfg, STTWContextRef* ctx){
    BOOL r = FALSE;
    STTWServerOpq* opq = (STTWServerOpq*)ref.opaque; NBASSERT(TWServer_isClass(ref))
    if(cfg != NULL && ctx != NULL && TWContext_isSet(*ctx) && !NBHttpService_isSet(opq->srv)){
        STNBStopFlagRef stopFlag;
        STNBThreadsPoolRef threads;
        STNBIOPollstersPoolRef pollsters;
        STNBIOPollstersProviderRef pollsProvider = NBIOPollstersProvider_alloc(NULL);
        NBMemory_setZeroSt(stopFlag, STNBStopFlagRef);
        NBMemory_setZeroSt(threads, STNBThreadsPoolRef);
        NBMemory_setZeroSt(pollsters, STNBIOPollstersPoolRef);
        {
            TWContext_getStopFlag(*ctx, &stopFlag);
            TWContext_getThreadsPool(*ctx, &threads);
            TWContext_getPollstersPool(*ctx, &pollsters);
            if(NBIOPollstersPool_isSet(pollsters)){
                if(!NBIOPollstersPool_linkToProvider(pollsters, &pollsProvider)){
                    PRINTF_ERROR("TWServer, NBIOPollstersPool_linkToProvider failed.\n");
                    r = FALSE;
                }
            }
        }
        //stats
        {
            const UI64 timestamp = NBDatetime_getCurUTCTimestamp();
            SI32 i; for (i = 0; i < (sizeof(opq->stats.dataPerFrame) / sizeof(opq->stats.dataPerFrame[0])); i++) {
                STTWServerStatsFrame* f = &opq->stats.dataPerFrame[i];
                f->frameStart = TWTimeframe_getBaseTimestamp((ENTWTimeframe)i, cfg->stats.atom, timestamp);
            }
        }
        //HttpService
        if(NBThreadsPool_isSet(threads) && NBIOPollstersPool_isSet(pollsters) && NBIOPollstersProvider_isSet(pollsProvider)){
            STNBHttpServiceRef srv = NBHttpService_alloc(NULL);
            NBObject_lock(opq);
            {
                r = TRUE;
                //stoFlag
                if(r){
                    NBStopFlag_setParentFlag(opq->stopFlag, &stopFlag);
                }
                //service
                if(r){
                    STNBHttpServiceLstnrItf itf;
                    NBMemory_setZeroSt(itf, STNBHttpServiceLstnrItf);
                    itf.httpCltConnected    = TWServer_httpCltConnected_;
                    itf.httpCltDisconnected = TWServer_httpCltDisconnected_;
                    itf.httpCltReqArrived   = TWServer_httpCltReqArrived_;
                    if(!NBHttpService_setPollstersProvider(srv, pollsProvider)){
                        PRINTF_ERROR("TWServer, NBHttpService_setPollstersProvider failed.\n");
                        r = FALSE;
                    } else if(!NBHttpService_setParentStopFlag(srv, &opq->stopFlag)){
                        PRINTF_ERROR("TWServer, NBHttpService_setParentStopFlag failed.\n");
                        r = FALSE;
                    } else if(!NBHttpService_prepare(srv, &cfg->http, &itf, opq)){
                        PRINTF_ERROR("TWServer, NBHttpService_prepare failed.\n");
                        r = FALSE;
                    } else {
                        PRINTF_INFO("TWServer, NBHttpService_prepare success.\n");
                        //consume
                        NBHttpService_swap(&opq->srv, &srv);
                    }
                }
                //copy cfg
                if(r){
                    NBStruct_stRelease(TWCfg_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
                    NBStruct_stClone(TWCfg_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
                }
            }
            NBObject_unlock(opq);
            //release (if not consumed)
            if(NBHttpService_isSet(srv)){
                NBHttpService_release(&srv);
                NBHttpService_null(&srv);
            }
        }
        if(NBStopFlag_isSet(stopFlag)){
            NBStopFlag_release(&stopFlag);
            NBStopFlag_null(&stopFlag);
        }
        if(NBThreadsPool_isSet(threads)){
            NBThreadsPool_release(&threads);
            NBThreadsPool_null(&threads);
        }
        if(NBIOPollstersPool_isSet(pollsters)){
            NBIOPollstersPool_release(&pollsters);
            NBIOPollstersPool_null(&pollsters);
        }
        if(NBIOPollstersProvider_isSet(pollsProvider)){
            NBIOPollstersProvider_release(&pollsProvider);
            NBIOPollstersProvider_null(&pollsProvider);
        }
    }
    return r;
}

BOOL TWServer_start(STTWServerRef ref){ //async
    BOOL r = FALSE;
    STTWServerOpq* opq = (STTWServerOpq*)ref.opaque; NBASSERT(TWServer_isClass(ref))
    NBObject_lock(opq);
    if(NBHttpService_isSet(opq->srv)){
        //
        NBStopFlag_reset(opq->stopFlag);
        //
        if(!NBHttpService_startListening(opq->srv)){
            PRINTF_ERROR("TWServer, NBHttpService_startListening failed.\n");
        } else {
            PRINTF_INFO("TWServer, NBHttpService_startListening success.\n");
            r = TRUE;
        }
    }
    NBObject_unlock(opq);
    return r;
}

void TWServer_printHttpStats_(const ENTWTimeframe iFrame, const SI64 secsRunning, STNBHttpStatsData* data);

BOOL TWServer_execute(STTWServerRef ref, const UI64 msToRun){ //blocking
    BOOL r = FALSE;
    STTWServerOpq* opq = (STTWServerOpq*)ref.opaque; NBASSERT(TWServer_isClass(ref))
    NBObject_lock(opq);
    if(NBHttpService_isSet(opq->srv)){
        //
        NBStopFlag_reset(opq->stopFlag);
        //
        if(!NBHttpService_startListening(opq->srv)){
            PRINTF_ERROR("TWServer, NBHttpService_startListening failed.\n");
        } else {
            PRINTF_INFO("TWServer, NBHttpService_startListening success.\n");
            r = TRUE;
            //block
            {
                
                UI64 msAccum = 0, secsRunning = 0;
                STNBTimestampMicro lastTime = NBTimestampMicro_getMonotonicFast();
                STNBHttpServiceCmdState statsFlushCmd; UI64 seqStatsFlushed = 0;
                NBMemory_setZeroSt(statsFlushCmd, STNBHttpServiceCmdState);
                while(!NBStopFlag_isAnyActivated(opq->stopFlag)){
                    //wait a second
                    NBThreadCond_timedWaitObj(&opq->cond, opq, 100);
                    //timed-execution
                    {
                        const STNBTimestampMicro nowTime = NBTimestampMicro_getMonotonicFast();
                        const SI64 msDiff = NBTimestampMicro_getDiffInMs(&lastTime, &nowTime);
                        if (msDiff < 0) {
                            //clock changed?
                            lastTime = nowTime;
                        } else if ((msAccum + msDiff) >= 1000) {
                            lastTime = nowTime;
                            secsRunning += (msAccum + msDiff) / 1000;
                            msAccum = (msAccum + msDiff) % 1000;
                            //PRINTF_INFO("TWServer, %lld secs running.\n", secsRunning);
                            //start stats-flush
                            if (NBHttpService_isSet(opq->srv)) {
                                NBHttpService_statsFlushStart(opq->srv, &statsFlushCmd);
                                //PRINTF_INFO("TWServer, stats-flush-cmd(%llu) started.\n", statsFlushCmd.seq);
                            }
                            //auto-stop
                            if (msToRun > 0 && secsRunning >= msToRun) {
                                NBStopFlag_activate(opq->stopFlag);
                            }
                        }
                    }
                    //analyze stats-flush completion
                    if (seqStatsFlushed != statsFlushCmd.seq) {
                        //analyze
                        if (statsFlushCmd.isPend) {
                            if (NBHttpService_isSet(opq->srv)) {
                                NBHttpService_statsFlushIsPend(opq->srv, &statsFlushCmd);
                                if (!statsFlushCmd.isPend) {
                                    //PRINTF_INFO("TWServer, stats-flush-cmd(%llu) completed.\n", statsFlushCmd.seq);
                                }
                            }
                        }
                        //get data
                        if (!statsFlushCmd.isPend) {
                            STNBHttpStatsData data;
                            NBMemory_setZeroSt(data, STNBHttpStatsData);
                            NBHttpService_statsGet(opq->srv, &data, TRUE /*reset*/);
                            //append data to frames
                            {
                                const UI64 timestamp = NBDatetime_getCurUTCTimestamp();
                                SI32 i; for (i = 0; i < (sizeof(opq->stats.dataPerFrame) / sizeof(opq->stats.dataPerFrame[0])); i++) {
                                    STTWServerStatsFrame* f = &opq->stats.dataPerFrame[i];
                                    const UI64 newFrameStart = TWTimeframe_getBaseTimestamp((ENTWTimeframe)i, opq->cfg.stats.atom, timestamp);
                                    if (f->frameStart != newFrameStart) {
                                        //print frame-stats
                                        TWServer_printHttpStats_((ENTWTimeframe)i, secsRunning , &f->data);
                                        //reset frame-data
                                        NBStruct_stRelease(NBHttpStatsData_getSharedStructMap(), &f->data, sizeof(f->data));
                                        f->frameStart = newFrameStart;
                                    }
                                    //accum data
                                    NBHttpStatsData_accumData(&f->data, &data);
                                }
                            }
                            NBStruct_stRelease(NBHttpStatsData_getSharedStructMap(), &data, sizeof(data));
                            //
                            seqStatsFlushed = statsFlushCmd.seq;
                            //PRINTF_INFO("TWServer, stats-flush-data(%llu) added.\n", statsFlushCmd.seq);
                        }
                    }
                }
            }
        }
    }
    NBObject_unlock(opq);
    return r;
}

void TWServer_printHttpStats_(const ENTWTimeframe iFrame, const SI64 secsRunning, STNBHttpStatsData* data) {
    const STNBEnumMap* enMap = TWTimeframe_getSharedEnumMap();
    BOOL elemsCount = 0;
    STNBString str;
    NBString_init(&str);
    //io
    {
        if (data->flow.bytes.in != 0 || data->flow.bytes.out != 0) {
            if (elemsCount != 0) NBString_concat(&str, ", ");
            if (data->flow.bytes.in != 0) {
                if (data->flow.bytes.in >= (1024 * 1024 * 1024)) {
                    NBString_concatDouble(&str, (double)data->flow.bytes.in / (double)(1024 * 1024 * 1024), 2);
                    NBString_concat(&str, " GBs rcvd");
                } else if (data->flow.bytes.in >= (1024 * 1024)) {
                    NBString_concatDouble(&str, (double)data->flow.bytes.in / (double)(1024 * 1024), 2);
                    NBString_concat(&str, " MBs rcvd");
                } else if (data->flow.bytes.in >= 1024) {
                    NBString_concatDouble(&str, (double)data->flow.bytes.in / (double)(1024), 2);
                    NBString_concat(&str, " KBs rcvd");
                } else {
                    NBString_concatUI64(&str, data->flow.bytes.in);
                    NBString_concat(&str, " bytes rcvd");
                }
            }
            if (data->flow.bytes.out != 0) {
                if (data->flow.bytes.in != 0) NBString_concat(&str, ", ");
                if (data->flow.bytes.out >= (1024 * 1024 * 1024)) {
                    NBString_concatDouble(&str, (double)data->flow.bytes.out / (double)(1024 * 1024 * 1024), 2);
                    NBString_concat(&str, " GBs sent");
                } else if (data->flow.bytes.out >= (1024 * 1024)) {
                    NBString_concatDouble(&str, (double)data->flow.bytes.out / (double)(1024 * 1024), 2);
                    NBString_concat(&str, " MBs sent");
                } else if (data->flow.bytes.out >= 1024) {
                    NBString_concatDouble(&str, (double)data->flow.bytes.out / (double)(1024), 2);
                    NBString_concat(&str, " KBs sent");
                } else {
                    NBString_concatUI64(&str, data->flow.bytes.out);
                    NBString_concat(&str, " bytes sent");
                }
            }
            elemsCount++;
        }
    }
    //conns
    if (data->flow.connsIn != 0 || data->flow.connsRejects != 0) {
        if (elemsCount != 0) NBString_concat(&str, ", ");
        if (data->flow.connsIn != 0) {
            NBString_concat(&str, "+");
            NBString_concatUI64(&str, data->flow.connsIn);
            NBString_concat(&str, " conns");
        }
        if (data->flow.connsRejects != 0) {
            if (data->flow.connsIn != 0) NBString_concat(&str, ", ");
            NBString_concat(&str, "+");
            NBString_concatUI64(&str, data->flow.connsRejects);
            NBString_concat(&str, " rejected");
        }
    }
    //reqs
    if (data->flow.requests != 0) {
        if (elemsCount != 0) NBString_concat(&str, ", ");
        NBString_concat(&str, "+");
        NBString_concatUI64(&str, data->flow.requests);
        NBString_concat(&str, " reqs");
    }
    if (str.length <= 0) {
        PRINTF_INFO("TWServer, (%s) %lld secs running.\n", enMap->records[iFrame].strValue, secsRunning);
    } else {
        PRINTF_INFO("TWServer, (%s) %lld secs running: %s.\n", enMap->records[iFrame].strValue, secsRunning, str.str);
    }
    NBString_release(&str);
}

//http

BOOL TWServer_httpCltConnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* usrData){ //new client connected
    BOOL r = TRUE; //keep-connection
    {
        //nothing
    }
    return r;
}

void TWServer_httpCltDisconnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* usrData){ //connection was closed or lost
    //ok
}

//TWServerFileResp

typedef struct STTWServerFileResp_ {
    STNBFileRef file;           //opened file
    STNBString  filePathName;   //includes path and name
    SI64        contentLenght;  //len when response started
    SI64        contentSent;    //len already sent
    //cfgs
    struct {
        STTWCfgWebPath* path;   //path's config
        STTWCfgWebPath* def;    //default config
    } cfgs;
    //read
    struct {
        SI32 iCsmd;             //consumed index (iCsmd <= iFilled)
        SI32 iFilled;           //filled index (iCsmd <= iFilled)
        BYTE buff[1024 * 64];
    } read;
} STTWServerFileResp;

void TWServerFileResp_init(STTWServerFileResp* obj);
void TWServerFileResp_release(STTWServerFileResp* obj);
BOOL TWServerFileResp_sendContent(STTWServerFileResp* obj, const STNBHttpServiceRespCtx ctx);

//http itf
void TWServerFileResp_httpReqOwnershipEnded_file_      (const STNBHttpServiceRespCtx ctx, void* usrData);    //request ended and none of these callbacks will be called again, release resources (not necesary to call 'httpReqRespClose')
BOOL TWServerFileResp_httpReqConsumeBodyEnd_file_      (const STNBHttpServiceRespCtx ctx, void* lparam);
void TWServerFileResp_httpReqTick_file_                (const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData);        //request poll-tick

//

BOOL TWServer_pathAddFileName_(STNBString* dst, const char* filename, const UI32 fileNameSz){
    BOOL r = TRUE;
    if(fileNameSz == 0){
        //empty file-name, ignore
    } else if(fileNameSz == 1 && filename[0] == '.'){
        //'.' file, ignore
    } else if(fileNameSz == 2 && filename[0] == '.' && filename[1] == '.'){
        //'..' file, remove previous file-name
        const SI32 lastSlashPos = NBString_strLastIndexOf(dst->str, "/", dst->length - 1);
        if(lastSlashPos < 0){
            r = FALSE;
        } else {
            NBString_removeLastBytes(dst, dst->length - lastSlashPos);
            NBASSERT(dst->length == 0 || dst->str[dst->length - 1] != '/') //should not end with '/'
        }
    } else {
        //add file-name
        NBString_concatByte(dst, '/');
        NBString_concatBytes(dst, filename, fileNameSz);
    }
    return r;
}

BOOL TWServer_httpCltReqArrived_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk, void* usrData){    //called when header-frist-line arrived, when header completion arrived or when body completing arrived; first to populate required methods into 'dstLtnr' take ownership and stops further calls to this method.
    STTWServerOpq* opq = (STTWServerOpq*)usrData; NBASSERT(TWServer_isClass(TWServer_fromOpqPtr(opq)))
    BOOL r = TRUE; //keep-connection
    if(reqDesc.header == NULL){
        //only first-line arrived so far
    } else if(reqDesc.body == NULL){
        //only header arrived so far
    } else {
        STNBString absPath, query, fragment;
        NBString_initWithSz(&absPath, 0, 64, 0.10f);
        NBString_initWithSz(&query, 0, 64, 0.10f);
        NBString_initWithSz(&fragment, 0, 64, 0.10f);
        if(opq == NULL){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Internal error, no OPQ-PTR")
                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Internal error, no OPQ-PTR");
        } else if(!NBHttpHeader_strParseRequestTarget(reqDesc.firstLine.target, &absPath, &query, &fragment)){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "No valid target")
                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "No valid target");
        } else {
            //search web-rules
            NBObject_lock(opq);
            {
                BOOL doResponse = TRUE;
                //search for rule
                
                const STTWCfgWebSite* site  = NULL;
                const STTWCfgHostPort* host = NULL;
                const char* pathRoot        = NULL;
                const STTWCfgWebPathChars* chars = NULL;
                const STTWCfgWebPathRuleChars* black = NULL;
                const STTWCfgWebPathRuleChars* white = NULL;
                char** defaultDocs = NULL;
                UI32  defaultDocsSz = 0;
                BOOL  describeFolders = FALSE;
                //
                STNBString filteredAbsPath;
                NBString_initWithSz(&filteredAbsPath, 0, 128, 0.10f);
                //select default rules for path
                {
                    if(opq->cfg.web.defaults != NULL){
                        const STTWCfgWebPath* path = opq->cfg.web.defaults;
                        if(path->root != NULL){
                            pathRoot = path->root;
                        }
                        if(path->chars != NULL){
                            chars = path->chars;
                            if(chars->black != NULL){
                                black = chars->black;
                            }
                            if(chars->white != NULL){
                                white = chars->white;
                            }
                        }
                        defaultDocs = path->defaultDocs;
                        defaultDocsSz = path->defaultDocsSz;
                        describeFolders = path->describeFolders;
                    }
                }
                //select by server value
                {
                    const char* hostFld = NBHttpHeader_getField(reqDesc.header, "host");
                    if(hostFld != NULL){
                        UI32 hostLen = 0; UI32 hostPort = 0; BOOL portIsExplicit = FALSE;
                        //parse 'host' field
                        {
                            const UI32 hostFldLen = NBString_strLenBytes(hostFld);
                            SI32 iPortPos = NBString_strLastIndexOf(hostFld, ":", hostFldLen - 1);
                            if(iPortPos < 0){
                                hostLen = hostFldLen; hostPort = 0; portIsExplicit = FALSE;
                            } else {
                                hostLen     = iPortPos;
                                hostPort    = NBNumParser_toUI32(&hostFld[iPortPos + 1], &portIsExplicit);
                                if(!portIsExplicit){
                                    doResponse = FALSE;
                                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Invalid host-port")
                                        && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Invalid host-port");
                                } else if(hostPort != port){
                                    doResponse = FALSE;
                                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Mismatched host-port")
                                        && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Mismatched host-port");
                                }
                            }
                        }
                        //search host rule
                        if(doResponse && opq->cfg.web.sites != NULL && opq->cfg.web.sitesSz > 0){
                            BOOL exactNameFnd = FALSE;
                            UI32 i, i2; for(i = 0; i < opq->cfg.web.sitesSz && !exactNameFnd; i++){
                                const STTWCfgWebSite* site2 = &opq->cfg.web.sites[i];
                                if(site2->hostnames != NULL){
                                    for(i2 = 0; i2 < site2->hostnamesSz; i2++){
                                        const STTWCfgHostPort* host2 = &site2->hostnames[i2];
                                        //Note: '*' is not supported as hostname, due to security reasons.
                                        const BOOL isExactName = !(hostLen == 1 && hostFld[0] == '*') && NBString_strIsLikeStrBytes(host2->name, hostFld, hostLen);
                                        if(
                                           (NBString_strIsEqual(host2->name, "*") || isExactName)
                                           && (host2->port == 0 || (host2->port == (portIsExplicit ? hostPort : port)))
                                           )
                                        {
                                            //apply rules
                                            site = site2;
                                            host = host2;
                                            if(site->path != NULL){
                                                const STTWCfgWebPath* path = site2->path;
                                                if(path->root != NULL){
                                                    pathRoot = path->root;
                                                }
                                                if(path->chars != NULL){
                                                    chars = path->chars;
                                                    if(chars->black != NULL){
                                                        black = chars->black;
                                                    }
                                                    if(chars->white != NULL){
                                                        white = chars->white;
                                                    }
                                                }
                                                defaultDocs = path->defaultDocs;
                                                defaultDocsSz = path->defaultDocsSz;
                                                describeFolders = path->describeFolders;
                                            }
                                            //
                                            if(isExactName){
                                                exactNameFnd = TRUE;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        //detect blacklisted chars
                        if(doResponse && black != NULL){
                            if(doResponse && !black->disableDefaults){
                                UI8 cLen; const char* cc = "^\"`\'";
                                while(*cc != '\0'){
                                    cLen = NBEncoding_utf8BytesExpected(*cc);
                                    if(NBString_strIndexOfBytes(absPath.str, absPath.length, cc, cLen) >= 0){
                                        doResponse = FALSE;
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Default-blacklisted-char found")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Default-blacklisted-char found");
                                        break;
                                    }
                                    cc += cLen;
                                }
                            }
                            if(doResponse && black->list != NULL){
                                UI8 cLen; const char* cc = black->list;
                                while(*cc != '\0'){
                                    cLen = NBEncoding_utf8BytesExpected(*cc);
                                    if(NBString_strIndexOfBytes(absPath.str, absPath.length, cc, cLen) >= 0){
                                        doResponse = FALSE;
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Explicit-blacklisted-char found")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Explicit-blacklisted-char found");
                                        break;
                                    }
                                    cc += cLen;
                                }
                            }
                        }
                        //detect whitelisted chars
                        if(doResponse && white != NULL){
                            if(doResponse && !white->disableDefaults){
                                //nothing
                            }
                            if(doResponse && white->list != NULL){
                                UI8 cLen; const char* cc = absPath.str;
                                const UI32 listLen = NBString_strLenBytes(white->list);
                                while(*cc != '\0'){
                                    cLen = NBEncoding_utf8BytesExpected(*cc);
                                    if(NBString_strIndexOfBytes(white->list, listLen, cc, cLen) >= 0){
                                        doResponse = FALSE;
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Char not in explicit-whitelist")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Char not in explicit-whitelist");
                                        break;
                                    }
                                    cc += cLen;
                                }
                            }
                        }
                        //build file path (validating '.' and '..' locations)
                        if(doResponse && absPath.length > 0){
                            const char* cCurFileStart = absPath.str;
                            UI8 cLen; const char* cc = absPath.str;
                            BOOL lastWasSlash = FALSE;
                            while(*cc != '\0'){
                                cLen = NBEncoding_utf8BytesExpected(*cc);
                                lastWasSlash = (*cc == '/' || *cc == '\\');
                                if(lastWasSlash){
                                    if(!TWServer_pathAddFileName_(&filteredAbsPath, cCurFileStart, (UI32)(cc - cCurFileStart))){
                                        doResponse = FALSE;
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Forbidden backward path")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Forbidden backward path");
                                        break;
                                    }
                                    cCurFileStart = (cc + cLen);
                                }
                                cc += cLen;
                            }
                            //add last file-name
                            if(doResponse){
                                if(!TWServer_pathAddFileName_(&filteredAbsPath, cCurFileStart, (UI32)(cc - cCurFileStart))){
                                    doResponse = FALSE;
                                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Forbidden backward path")
                                        && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Forbidden backward path");
                                }
                                if(lastWasSlash){
                                    NBString_concatByte(&filteredAbsPath, '/');
                                }
                            }
                        }
                        //domain-level redirection
                        if(doResponse && host != NULL && host->redirect != NULL && host->redirect->protocol != NULL){
                            UI32 redirErrCode = 0;
                            STNBString redirLoc;
                            NBString_initWithSz(&redirLoc, 0, 128, 0.10f);
                            //protocol
                            NBString_concat(&redirLoc, host->redirect->protocol);
                            NBString_concat(&redirLoc, "://");
                            //host
                            if(host->redirect->host != NULL){
                                //explicit redirect host
                                NBString_concat(&redirLoc, host->redirect->host);
                            } else {
                                //implicit redirect host (host header)
                                const char* hostFld = NBHttpHeader_getField(reqDesc.header, "host");
                                if(hostFld == NULL){
                                    //error
                                    redirErrCode = 400;
                                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, redirErrCode, "Host-header is required")
                                        && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Host-header is required");
                                    doResponse = FALSE;
                                } else {
                                    UI32 hostLen = 0;
                                    //parse 'host' field
                                    const UI32 hostFldLen = NBString_strLenBytes(hostFld);
                                    SI32 iPortPos = NBString_strLastIndexOf(hostFld, ":", hostFldLen - 1);
                                    if(iPortPos < 0){
                                        hostLen = hostFldLen;
                                    } else {
                                        hostLen = iPortPos;
                                    }
                                    if(hostLen == 0){
                                        //error
                                        redirErrCode = 400;
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, redirErrCode, "Empty host-header")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Empty host-header");
                                        doResponse = FALSE;
                                    } else {
                                        NBString_concatBytes(&redirLoc, hostFld, hostLen);
                                    }
                                }
                            }
                            //complete
                            if(redirErrCode == 0){
                                //port
                                if(host->redirect->port > 0){
                                    NBString_concatByte(&redirLoc, ':');
                                    NBString_concatUI32(&redirLoc, host->redirect->port);
                                }
                                //target
                                NBString_concatBytes(&redirLoc, absPath.str, absPath.length);
                                if(query.length > 0){
                                    NBString_concatByte(&redirLoc, '?');
                                    NBString_concatBytes(&redirLoc, query.str, query.length);
                                }
                                if(fragment.length > 0){
                                    NBString_concatByte(&redirLoc, '#');
                                    NBString_concatBytes(&redirLoc, fragment.str, fragment.length);
                                }
                                //
                                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 301, "Moved Permanently")
                                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Moved Permanently")
                                    && NBHttpServiceReqArrivalLnk_setDefaultResponseField(&reqLnk, "Location", redirLoc.str);
                                //
                                //PRINTF_INFO("Redirecting from port(%d) res('%s%s%s%s%s') to '%s'.\n", port, absPath.str, query.length > 0 ? "?" : "", query.str, fragment.length > 0 ? "#" : "", fragment.str, redirLoc.str);
                                doResponse = FALSE;
                            }
                            //
                            NBString_release(&redirLoc);
                        }
                        //default-doc redirection ("/path/index.html" -> "/path/")
                        if(doResponse && host != NULL && defaultDocs != NULL && defaultDocsSz > 0){
                            const SI32 iLastSlash = NBString_lastIndexOf(&filteredAbsPath, "/", filteredAbsPath.length - 1);
                            if(iLastSlash >= 0 && (iLastSlash + 1) < filteredAbsPath.length){
                                BOOL isDefDocMatch = FALSE;
                                //analyze if requested file would match a default-doc result
                                {
                                    const char* lstFile = &filteredAbsPath.str[iLastSlash + 1];
                                    STNBString pathTmp;
                                    NBString_init(&pathTmp);
                                    SI32 i; for(i = 0; i < defaultDocsSz; ++i){
                                        if(NBString_strIsEqual(lstFile, defaultDocs[i])){
                                            //default-doc match
                                            STNBFileRef file = NBFile_alloc(NULL);
                                            //build file-path
                                            {
                                                UI32 absPathStart = 0;
                                                NBString_set(&pathTmp, pathRoot); NBASSERT(pathTmp.length > 0)
                                                if(pathTmp.str[pathTmp.length - 1] == '/' && filteredAbsPath.str[0] == '/'){
                                                    //ignore starting-slash if root-path already ends with an slash.
                                                    absPathStart = 1;
                                                }
                                                NBString_concatBytes(&pathTmp, &filteredAbsPath.str[absPathStart], iLastSlash + 1 - absPathStart);
                                                NBString_concat(&pathTmp, defaultDocs[i]);
                                            }
                                            //search file
                                            if(NBFile_open(file, pathTmp.str, ENNBFileMode_Read)){
                                                //requested default-doc exists
                                                //analyze if a previous default-doc would have priority over this one.
                                                isDefDocMatch = TRUE;
                                                --i; for(; i >= 0 && isDefDocMatch; --i){
                                                    STNBFileRef file = NBFile_alloc(NULL);
                                                    //build file-path
                                                    {
                                                        UI32 absPathStart = 0;
                                                        NBString_set(&pathTmp, pathRoot); NBASSERT(pathTmp.length > 0)
                                                        if(pathTmp.str[pathTmp.length - 1] == '/' && filteredAbsPath.str[0] == '/'){
                                                            //ignore starting-slash if root-path already ends with an slash.
                                                            absPathStart = 1;
                                                        }
                                                        NBString_concatBytes(&pathTmp, &filteredAbsPath.str[absPathStart], iLastSlash + 1 - absPathStart);
                                                        NBString_concat(&pathTmp, defaultDocs[i]);
                                                    }
                                                    //search file
                                                    if(NBFile_open(file, pathTmp.str, ENNBFileMode_Read)){
                                                        //do not redirect to folder path, because another default-doc would be server.
                                                        //keep current default-doc as the explicit requested document.
                                                        isDefDocMatch = FALSE;
                                                    }
                                                    NBFile_release(&file);
                                                    NBFile_null(&file);
                                                }
                                            }
                                            NBFile_release(&file);
                                            NBFile_null(&file);
                                            break;
                                        }
                                    }
                                    NBString_release(&pathTmp);
                                }
                                //redirect by removing default-doc from path
                                if(isDefDocMatch){
                                    const BOOL isSslEnabled = NBHttpServiceConn_isSslEnabled(conn);
                                    UI32 redirErrCode = 0;
                                    STNBString redirLoc;
                                    NBString_initWithSz(&redirLoc, 0, 128, 0.10f);
                                    //protocol
                                    NBString_concat(&redirLoc, isSslEnabled ? "https" : "http");
                                    NBString_concat(&redirLoc, "://");
                                    //host
                                    {
                                        const char* hostFld = NBHttpHeader_getField(reqDesc.header, "host");
                                        if(hostFld == NULL){
                                            //error
                                            redirErrCode = 400;
                                            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, redirErrCode, "Host-header is required")
                                                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Host-header is required");
                                            doResponse = FALSE;
                                        } else {
                                            UI32 hostLen = 0;
                                            //parse 'host' field
                                            const UI32 hostFldLen = NBString_strLenBytes(hostFld);
                                            SI32 iPortPos = NBString_strLastIndexOf(hostFld, ":", hostFldLen - 1);
                                            if(iPortPos < 0){
                                                hostLen = hostFldLen;
                                            } else {
                                                hostLen = iPortPos;
                                            }
                                            if(hostLen == 0){
                                                //error
                                                redirErrCode = 400;
                                                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, redirErrCode, "Empty host-header")
                                                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Empty host-header");
                                                doResponse = FALSE;
                                            } else {
                                                NBString_concatBytes(&redirLoc, hostFld, hostLen);
                                            }
                                        }
                                    }
                                    //complete
                                    if(redirErrCode == 0){
                                        //port
                                        if(port != (isSslEnabled ? 443 : 80)){
                                            NBString_concatByte(&redirLoc, ':');
                                            NBString_concatUI32(&redirLoc, port);
                                        }
                                        //target
                                        NBString_concatBytes(&redirLoc, filteredAbsPath.str, iLastSlash + 1); //just up to the last slash
                                        if(query.length > 0){
                                            NBString_concatByte(&redirLoc, '?');
                                            NBString_concatBytes(&redirLoc, query.str, query.length);
                                        }
                                        if(fragment.length > 0){
                                            NBString_concatByte(&redirLoc, '#');
                                            NBString_concatBytes(&redirLoc, fragment.str, fragment.length);
                                        }
                                        //
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 301, "Moved Permanently")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Moved Permanently")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseField(&reqLnk, "Location", redirLoc.str);
                                        //
                                        //PRINTF_INFO("Redirecting from port(%d) res('%s%s%s%s%s') to '%s'.\n", port, absPath.str, query.length > 0 ? "?" : "", query.str, fragment.length > 0 ? "#" : "", fragment.str, redirLoc.str);
                                        doResponse = FALSE;
                                    }
                                    //
                                    NBString_release(&redirLoc);
                                }
                            }
                        }
                        //search for file
                        if(doResponse){
                            if(NBString_strIsEmpty(pathRoot)){
                                doResponse = FALSE;
                                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Host-path not configured")
                                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Host-path not configured");
                            } else {
                                STTWServerFileResp* resp = NBMemory_allocType(STTWServerFileResp);
                                TWServerFileResp_init(resp);
                                //
                                if(site != NULL){
                                    resp->cfgs.path = site->path;
                                    resp->cfgs.def  = opq->cfg.web.defaults;
                                }
                                //
                                {
                                    STNBString filepath;
                                    NBString_init(&filepath);
                                    NBString_concat(&filepath, pathRoot); NBASSERT(filepath.length > 0)
                                    {
                                        const char* extraPath = filteredAbsPath.str;
                                        if(filepath.str[filepath.length - 1] == '/' && extraPath[0] == '/'){
                                            //ignore starting-slash if root-path already ends with an slash.
                                            extraPath++;
                                        }
                                        NBString_concat(&filepath, extraPath);
                                    }
                                    PRINTF_INFO("TWServer, serving file-path '%s' (base '%s' + target '%s').\n", filepath.str, pathRoot, filteredAbsPath.str);
                                    //serve requested resource
                                    if(resp != NULL && filepath.length > 0){
                                        if(filepath.str[filepath.length - 1] == '/' || filepath.str[filepath.length - 1] == '\\'){
                                            //serve as a folder-path
                                            //Find defaultDoc that matches
                                            if(resp != NULL && defaultDocs != NULL && defaultDocsSz > 0){
                                                STNBString pathTmp;
                                                NBString_init(&pathTmp);
                                                UI32 i; for(i = 0; i < defaultDocsSz && resp != NULL; i++){
                                                    const char* defDoc = defaultDocs[i];
                                                    if(!NBString_strIsEmpty(defDoc) && !NBString_strIsEqual(defDoc, ".") && !NBString_strIsEqual(defDoc, "..") && NBString_strIndexOf(defDoc, "/", 0) < 0 && NBString_strIndexOf(defDoc, "\\", 0) < 0){
                                                        NBString_set(&pathTmp, filepath.str);
                                                        if(!(filepath.str[filepath.length - 1] == '/' || filepath.str[filepath.length - 1] == '\\')){
                                                            NBString_concatByte(&pathTmp, '/');
                                                        }
                                                        NBString_concat(&pathTmp, defDoc);
                                                        if(!NBFile_open(resp->file, pathTmp.str, ENNBFileMode_Read)){
                                                            //PRINTF_INFO("TWServer, defaultDoc not found: '%s'.\n", pathTmp.str);
                                                            NBFile_release(&resp->file);
                                                            resp->file = NBFile_alloc(NULL);
                                                        } else {
                                                            STNBHttpServiceReqLstnrItf itf;
                                                            NBMemory_setZeroSt(itf, STNBHttpServiceReqLstnrItf);
                                                            itf.httpReqOwnershipEnded = TWServerFileResp_httpReqOwnershipEnded_file_;
                                                            itf.httpReqConsumeBodyEnd = TWServerFileResp_httpReqConsumeBodyEnd_file_;
                                                            itf.httpReqTick = TWServerFileResp_httpReqTick_file_;
                                                            NBString_setBytes(&resp->filePathName, pathTmp.str, pathTmp.length);
                                                            //PRINTF_INFO("TWServer, defaultDoc found: '%s'.\n", pathTmp.str);
                                                            if(!NBHttpServiceReqArrivalLnk_setOwner(&reqLnk, &itf, resp, 0)){
                                                                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Internal error, req-not-owned")
                                                                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Internal error, req-not-owned");
                                                                //consume
                                                                TWServerFileResp_release(resp);
                                                                NBMemory_free(resp);
                                                                resp = NULL;
                                                            } else {
                                                                r = TRUE;
                                                                //consume
                                                                resp = NULL;
                                                            }
                                                            break;
                                                        }
                                                    }
                                                }
                                                NBString_release(&pathTmp);
                                            }
                                            //No default file found, describe folder (if allowed)
                                            if(resp != NULL && describeFolders){
                                                if(NBFilesystem_folderExists(&opq->fs, filepath.str)){
                                                    STNBString str;
                                                    NBString_init(&str);
                                                    {
                                                        NBString_concat(&str, "<html>");
                                                        NBString_concat(&str, "<head>");
                                                        NBString_concat(&str, "<title>");
                                                        NBString_concat(&str, filteredAbsPath.str);
                                                        NBString_concat(&str, "</title>");
                                                        NBString_concat(&str, "</head>");
                                                        NBString_concat(&str, "<body>");
                                                        NBString_concat(&str, "This is TWServer at ");
                                                        NBString_concatSqlDatetime(&str, NBDatetime_getCurLocal());
                                                        NBString_concat(&str, "'!");
                                                        {
                                                            const char* hostFld = NBHttpHeader_getField(reqDesc.header, "host");
                                                            if(hostFld == NULL){
                                                                NBString_concat(&str, "<br/>\n<br/>\nHost header-field: none.");
                                                            } else {
                                                                NBString_concat(&str, "\n<br/>\n<br/>Host header-field: '");
                                                                NBString_concat(&str, hostFld);
                                                                NBString_concat(&str, "'");
                                                            }
                                                        }
                                                        {
                                                            NBString_concat(&str, "<br/>\n<br/>\nFiltered abs path: '");
                                                            NBString_concat(&str, filteredAbsPath.str);
                                                            NBString_concat(&str, "'");
                                                            NBString_concat(&str, "<br/>\n");
                                                        }
                                                        //parent
                                                        {
                                                            NBString_concat(&str, "<br/>\n<a href=\"./\">./</a>");
                                                            NBString_concat(&str, "<br/>\n<a href=\"../\">../</a>");
                                                        }
                                                        //folders
                                                        {
                                                            STNBString strs;
                                                            STNBArray arr; //STNBFilesystemFile
                                                            NBArray_init(&arr, sizeof(STNBFilesystemFile), NULL);
                                                            NBString_init(&strs);
                                                            if(!NBFilesystem_getFolders(&opq->fs, filepath.str, &strs, &arr)){
                                                                NBString_concat(&str, "<br/>\n<br/>\nCould not retrieve folders.");
                                                            } else {
                                                                SI32 i; for(i = 0; i < arr.use; i++){
                                                                    const STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&arr, STNBFilesystemFile, i);
                                                                    const char* name = &strs.str[f->name];
                                                                    NBString_concat(&str, "<br/>\n");
                                                                    NBString_concat(&str, "<a href=\"./"); NBString_concat(&str, name); NBString_concat(&str, "/\">");
                                                                    NBString_concat(&str, name);
                                                                    NBString_concat(&str, "/");
                                                                    NBString_concat(&str, "</a>");
                                                                }
                                                            }
                                                            NBString_release(&strs);
                                                            NBArray_release(&arr);
                                                        }
                                                        //files
                                                        {
                                                            STNBString strs;
                                                            STNBArray arr; //STNBFilesystemFile
                                                            NBArray_init(&arr, sizeof(STNBFilesystemFile), NULL);
                                                            NBString_init(&strs);
                                                            if(!NBFilesystem_getFiles(&opq->fs, filepath.str, FALSE /*includeStats*/, &strs, &arr)){
                                                                NBString_concat(&str, "<br/>\n<br/>\nCould not retrieve files.");
                                                            } else {
                                                                SI32 i; for(i = 0; i < arr.use; i++){
                                                                    const STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&arr, STNBFilesystemFile, i);
                                                                    const char* name = &strs.str[f->name];
                                                                    NBString_concat(&str, "<br/>\n");
                                                                    NBString_concat(&str, "<a href=\"./"); NBString_concat(&str, name); NBString_concat(&str, "\">");
                                                                    NBString_concat(&str, name);
                                                                    NBString_concat(&str, "</a>");
                                                                }
                                                            }
                                                            NBString_release(&strs);
                                                            NBArray_release(&arr);
                                                        }
                                                        NBString_concat(&str, "</body>");
                                                        NBString_concat(&str, "</html>");
                                                        //
                                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 200, "OK")
                                                        && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, str.str);
                                                    }
                                                    NBString_release(&str);
                                                    //consume
                                                    TWServerFileResp_release(resp);
                                                    NBMemory_free(resp);
                                                    resp = NULL;
                                                }
                                            }
                                        } else if(NBFilesystem_folderExists(&opq->fs, filepath.str)){
                                            //redirect to folfer-path (add '/')
                                            const BOOL isSslEnabled = NBHttpServiceConn_isSslEnabled(conn);
                                            UI32 redirErrCode = 0;
                                            STNBString redirLoc;
                                            NBString_initWithSz(&redirLoc, 0, 128, 0.10f);
                                            //protocol
                                            NBString_concat(&redirLoc, isSslEnabled ? "https" : "http");
                                            NBString_concat(&redirLoc, "://");
                                            //host
                                            {
                                                const char* hostFld = NBHttpHeader_getField(reqDesc.header, "host");
                                                if(hostFld == NULL){
                                                    //error
                                                    redirErrCode = 400;
                                                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, redirErrCode, "Host-header is required")
                                                        && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Host-header is required");
                                                    //consume
                                                    TWServerFileResp_release(resp);
                                                    NBMemory_free(resp);
                                                    resp = NULL;
                                                } else {
                                                    UI32 hostLen = 0;
                                                    //parse 'host' field
                                                    const UI32 hostFldLen = NBString_strLenBytes(hostFld);
                                                    SI32 iPortPos = NBString_strLastIndexOf(hostFld, ":", hostFldLen - 1);
                                                    if(iPortPos < 0){
                                                        hostLen = hostFldLen;
                                                    } else {
                                                        hostLen = iPortPos;
                                                    }
                                                    if(hostLen == 0){
                                                        //error
                                                        redirErrCode = 400;
                                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, redirErrCode, "Empty host-header")
                                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Empty host-header");
                                                        //consume
                                                        TWServerFileResp_release(resp);
                                                        NBMemory_free(resp);
                                                        resp = NULL;
                                                    } else {
                                                        NBString_concatBytes(&redirLoc, hostFld, hostLen);
                                                    }
                                                }
                                            }
                                            //complete
                                            if(redirErrCode == 0){
                                                //port
                                                if(port != (isSslEnabled ? 443 : 80)){
                                                    NBString_concatByte(&redirLoc, ':');
                                                    NBString_concatUI32(&redirLoc, port);
                                                }
                                                //target
                                                NBString_concat(&redirLoc, filteredAbsPath.str);
                                                //concat slash '/'
                                                NBString_concatByte(&redirLoc, '/');
                                                //
                                                if(query.length > 0){
                                                    NBString_concatByte(&redirLoc, '?');
                                                    NBString_concatBytes(&redirLoc, query.str, query.length);
                                                }
                                                if(fragment.length > 0){
                                                    NBString_concatByte(&redirLoc, '#');
                                                    NBString_concatBytes(&redirLoc, fragment.str, fragment.length);
                                                }
                                                //
                                                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 301, "Moved Permanently")
                                                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Moved Permanently, redirecting to folder path.")
                                                    && NBHttpServiceReqArrivalLnk_setDefaultResponseField(&reqLnk, "Location", redirLoc.str);
                                                //
                                                PRINTF_INFO("Redirecting from port(%d) res('%s%s%s%s%s') to '%s'.\n", port, absPath.str, query.length > 0 ? "?" : "", query.str, fragment.length > 0 ? "#" : "", fragment.str, redirLoc.str);
                                                //consume
                                                TWServerFileResp_release(resp);
                                                NBMemory_free(resp);
                                                resp = NULL;
                                            }
                                            //
                                            NBString_release(&redirLoc);
                                        } else if(NBFile_open(resp->file, filepath.str, ENNBFileMode_Read)){
                                            //file-found
                                            STNBHttpServiceReqLstnrItf itf;
                                            NBMemory_setZeroSt(itf, STNBHttpServiceReqLstnrItf);
                                            itf.httpReqOwnershipEnded = TWServerFileResp_httpReqOwnershipEnded_file_;
                                            itf.httpReqConsumeBodyEnd = TWServerFileResp_httpReqConsumeBodyEnd_file_;
                                            itf.httpReqTick = TWServerFileResp_httpReqTick_file_;
                                            NBString_setBytes(&resp->filePathName, filepath.str, filepath.length);
                                            if(!NBHttpServiceReqArrivalLnk_setOwner(&reqLnk, &itf, resp, 0)){
                                                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Internal error, req-not-owned")
                                                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Internal error, req-not-owned");
                                                //consume
                                                TWServerFileResp_release(resp);
                                                NBMemory_free(resp);
                                                resp = NULL;
                                            } else {
                                                r = TRUE;
                                                //consume
                                                resp = NULL;
                                            }
                                        }
                                    }
                                    NBString_release(&filepath);
                                }
                                //default 404 response
                                if(resp != NULL){
                                    STNBString str;
                                    NBString_init(&str);
                                    {
                                        NBString_concat(&str, "404 - Not Found");
                                        //
                                        NBString_concat(&str, "\n\nThis is TWServer at ");
                                        NBString_concatSqlDatetime(&str, NBDatetime_getCurLocal());
                                        NBString_concat(&str, "'!");
                                        {
                                            const char* hostFld = NBHttpHeader_getField(reqDesc.header, "host");
                                            if(hostFld == NULL){
                                                NBString_concat(&str, "\n\nHost header-field: none.");
                                            } else {
                                                NBString_concat(&str, "\n\nHost header-field: '");
                                                NBString_concat(&str, hostFld);
                                                NBString_concat(&str, "'");
                                            }
                                        }
                                        {
                                            NBString_concat(&str, "\n\nFiltered abs path: '");
                                            NBString_concat(&str, filteredAbsPath.str);
                                            NBString_concat(&str, "'");
                                        }
                                        //
                                        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 404, "Not found")
                                            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, str.str);
                                    }
                                    NBString_release(&str);
                                    //consume
                                    TWServerFileResp_release(resp);
                                    NBMemory_free(resp);
                                    resp = NULL;
                                }
                            }
                        }
                    }
                }
                NBString_release(&filteredAbsPath);
            }
            NBObject_unlock(opq);
        }
        NBString_release(&fragment);
        NBString_release(&query);
        NBString_release(&absPath);
    }
    return r;
}

//TWServerFileResp

void TWServerFileResp_init(STTWServerFileResp* obj){
    NBMemory_setZeroSt(*obj, STTWServerFileResp);
    obj->file = NBFile_alloc(NULL);
    NBString_initWithSz(&obj->filePathName, 0, 128, 0.1f);
}

void TWServerFileResp_release(STTWServerFileResp* obj){
    NBString_release(&obj->filePathName);
    {
        NBFile_close(obj->file);
        NBFile_release(&obj->file);
        NBFile_null(&obj->file);
    }
}

BOOL TWServerFileResp_sendContent(STTWServerFileResp* obj, const STNBHttpServiceRespCtx ctx){
    BOOL r = TRUE;
    while(obj->contentSent < obj->contentLenght){
        //fill buffer
        if(obj->read.iFilled < sizeof(obj->read.buff)){
            const SI64 bytesRemainExpect = (obj->contentLenght - obj->contentSent);
            const SI64 bytesCanFill = (sizeof(obj->read.buff) - obj->read.iFilled);
            const UI32 bytesToRead = (UI32)(bytesRemainExpect < bytesCanFill ? bytesRemainExpect : bytesCanFill );
            if(NBFile_read(obj->file, &obj->read.buff[obj->read.iCsmd], bytesToRead) != bytesToRead){
                PRINTF_ERROR("TWServerFileResp, NBFile_read(%d bytes) failed.\n", bytesToRead);
                r = FALSE; //error, file is smaller than expected
                break;
            } else {
                obj->read.iFilled += bytesToRead;
            }
        }
        //send buffer
        if(obj->read.iCsmd < obj->read.iFilled){
            if(!NBHttpServiceRespLnk_concatBodyBytes(&ctx.resp.lnk, &obj->read.buff[obj->read.iCsmd], obj->read.iFilled - obj->read.iCsmd)){
                PRINTF_ERROR("TWServerFileResp, NBHttpServiceRespLnk_concatBodyBytes(%d bytes) failed.\n", (obj->read.iFilled - obj->read.iCsmd));
                r = FALSE; //error, something went wrong
                break;
            } else {
                PRINTF_INFO("TWServerFileResp, %lld of %lld bytes sent.\n", obj->contentSent, obj->contentLenght);
                obj->contentSent += (obj->read.iFilled - obj->read.iCsmd);
                obj->read.iCsmd = obj->read.iFilled = 0;
            }
        }
    }
    return r;
}

//http itf
void TWServerFileResp_httpReqOwnershipEnded_file_(const STNBHttpServiceRespCtx ctx, void* usrData){    //request ended and none of these callbacks will be called again, release resources (not necesary to call 'httpReqRespClose')
    STTWServerFileResp* resp = (STTWServerFileResp*)usrData;
    //free
    TWServerFileResp_release(resp);
    NBMemory_free(resp);
    resp = NULL;
}

BOOL TWServerFileResp_httpReqConsumeBodyEnd_file_(const STNBHttpServiceRespCtx ctx, void* usrData){
    BOOL r = TRUE;
    STTWServerFileResp* resp = (STTWServerFileResp*)usrData;
    //send response header
    if(!NBFile_isOpen(resp->file)){
        //something went wrong
        PRINTF_ERROR("TWServerFileResp, NBFile_isOpen failed.\n");
        r = FALSE;
    } else {
        NBFile_lock(resp->file);
        if(!NBFile_seek(resp->file, 0, ENNBFileRelative_End)){
            //something went wrong
            PRINTF_ERROR("TWServerFileResp, NBFile_seek(ENNBFileRelative_End) failed.\n");
            r = FALSE;
        } else {
            const SI64 fileSz = NBFile_curPos(resp->file);
            if(fileSz < 0){
                //something went wrong
                PRINTF_ERROR("TWServerFileResp, NBFile_curPos failed.\n");
                r = FALSE;
            } else if(!NBFile_seek(resp->file, 0, ENNBFileRelative_Start)){
                //something went wrong
                PRINTF_ERROR("TWServerFileResp, NBFile_seek(ENNBFileRelative_Start) failed.\n");
                r = FALSE;
            } else {
                const char* extNoDot = NULL;
                const char* mimeType = NULL;
                NBASSERT(resp->contentSent == 0)
                resp->contentSent = 0;
                resp->contentLenght = fileSz;
                //extNoDot
                if(resp->filePathName.length > 0){
                    const SI32 lastDotPos = NBString_lastIndexOf(&resp->filePathName, ".", resp->filePathName.length - 1);
                    if(lastDotPos >= 0){
                        extNoDot = &resp->filePathName.str[lastDotPos + 1];
                    }
                }
                //search mime-type
                if(extNoDot != NULL){
                    BOOL ignoreDefaultsMime = FALSE;
                    //PRINTF_INFO("Extension: '%s'.\n", extNoDot);
                    //path's config
                    if(mimeType == NULL && resp->cfgs.path != NULL && resp->cfgs.path->mimeTypes != NULL){
                        const STTWCfgMimeType* type = TWCfgMimeTypes_getTypeByExt(resp->cfgs.path->mimeTypes, extNoDot);
                        if(type != NULL){
                            //PRINTF_INFO("Mime-type: '%s' (by path's cfg).\n", type->mime);
                            mimeType = type->mime;
                        }
                        ignoreDefaultsMime = (ignoreDefaultsMime || resp->cfgs.path->mimeTypes->ignoreDefaults);
                    }
                    //default's config
                    if(mimeType == NULL && resp->cfgs.def != NULL && resp->cfgs.def->mimeTypes != NULL){
                        const STTWCfgMimeType* type = TWCfgMimeTypes_getTypeByExt(resp->cfgs.def->mimeTypes, extNoDot);
                        if(type != NULL){
                            //PRINTF_INFO("Mime-type: '%s' (by default's cfg).\n", type->mime);
                            mimeType = type->mime;
                        }
                        ignoreDefaultsMime = (ignoreDefaultsMime || resp->cfgs.def->mimeTypes->ignoreDefaults);
                    }
                    //defaults values (hardcoded)
                    if(mimeType == NULL && !ignoreDefaultsMime){
                        mimeType = TWMimeTypesDefaults_getByExt(extNoDot);
                        //PRINTF_INFO("Mime-type: '%s' (by default types).\n", mimeType);
                    }
                }
                //send header
                if(!NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 200, "OK")){
                    //something went wrong
                    PRINTF_ERROR("TWServerFileResp, NBHttpServiceRespLnk_setResponseCode failed.\n");
                    r = FALSE;
                } else if(mimeType != NULL && !NBHttpServiceRespLnk_setContentType(&ctx.resp.lnk, mimeType)){
                    //something went wrong
                    PRINTF_ERROR("TWServerFileResp, NBHttpServiceRespLnk_setContentType('%s') failed.\n", mimeType);
                    r = FALSE;
                } else if(!NBHttpServiceRespLnk_setContentLength(&ctx.resp.lnk, (UI64)fileSz)){
                    //something went wrong
                    PRINTF_ERROR("TWServerFileResp, NBHttpServiceRespLnk_setContentLength failed.\n");
                    r = FALSE;
                } else if(!NBHttpServiceRespLnk_endHeader(&ctx.resp.lnk)){
                    //something went wrong
                    PRINTF_ERROR("TWServerFileResp, NBHttpServiceRespLnk_endHeader failed.\n");
                    r = FALSE;
                } else {
                    //send body (start)
                    r = TRUE;
                    while(r && resp->contentSent < resp->contentLenght){
                        r = TWServerFileResp_sendContent(resp, ctx);
                    }
                    //end
                    if(!r || resp->contentSent == resp->contentLenght){
                        NBHttpServiceRespLnk_close(&ctx.resp.lnk);
                    }
                }
            }
        }
        NBFile_unlock(resp->file);
    }
    return r;
}

void TWServerFileResp_httpReqTick_file_(const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData){        //request poll-tick
    STTWServerFileResp* resp = (STTWServerFileResp*)usrData;
    //send body (continuation)
    BOOL r = TRUE;
    NBFile_lock(resp->file);
    {
        while(r && resp->contentSent < resp->contentLenght){
            r = TWServerFileResp_sendContent(resp, ctx);
        }
        //end (in case of error or completed0
        if(!r || resp->contentSent == resp->contentLenght){
            NBHttpServiceRespLnk_close(&ctx.resp.lnk);
        }
    }
    NBFile_unlock(resp->file);
}

//

void TWServerStatsFrame_init(STTWServerStatsFrame* obj) {
    NBMemory_setZeroSt(*obj, STTWServerStatsFrame);
}

void TWServerStatsFrame_release(STTWServerStatsFrame* obj) {
    NBStruct_stRelease(NBHttpStatsData_getSharedStructMap(), &obj->data, sizeof(obj->data));
}
