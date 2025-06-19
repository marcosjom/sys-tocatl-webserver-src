//
//  TWContext.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 29/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/TWContext.h"
//
#include "nb/core/NBStruct.h"

//TWContextOpq

typedef struct STTWContextOpq_ {
    STNBObject      prnt;
    STTWCfgContext  cfg;
    STNBStopFlagRef stopFlag;
    //threads
    struct {
        STNBThreadsPoolRef  pool;
    } threads;
    //pollsters
    struct {
        STNBIOPollstersPoolRef pool;
    } pollsters;
} STTWContextOpq;

NB_OBJREF_BODY(TWContext, STTWContextOpq, NBObject);

//TWContext

void TWContext_initZeroed(STNBObject* obj) {
    STTWContextOpq* opq = (STTWContextOpq*)obj;
    opq->stopFlag = NBStopFlag_alloc(NULL);
    //cfg
    {
        //nothing
    }
    //threads
    {
        //nothing
    }
    //pollsters
    {
        //nothing
    }
}

void TWContext_uninitLocked(STNBObject* obj){
    STTWContextOpq* opq = (STTWContextOpq*)obj;
    //
    NBStopFlag_activate(opq->stopFlag);
    //cfg
    {
        NBStruct_stRelease(TWCfgContext_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
    }
    //pollsters
    {
        if(NBIOPollstersPool_isSet(opq->pollsters.pool)){
            NBIOPollstersPool_release(&opq->pollsters.pool);
            NBIOPollstersPool_null(&opq->pollsters.pool);
        }
    }
    //threads
    {
        if(NBThreadsPool_isSet(opq->threads.pool)){
            NBThreadsPool_release(&opq->threads.pool);
            NBThreadsPool_null(&opq->threads.pool);
        }
    }
    //
    if(NBStopFlag_isSet(opq->stopFlag)){
        NBStopFlag_release(&opq->stopFlag);
        NBStopFlag_null(&opq->stopFlag);
    }
}

//


BOOL TWContext_prepare(STTWContextRef ref, const STTWCfgContext* cfg, STNBStopFlagRef* parentStopFlag){
    BOOL r = FALSE;
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    if(cfg != NULL && !NBThreadsPool_isSet(opq->threads.pool) && !NBIOPollstersPool_isSet(opq->pollsters.pool)){
        STNBThreadsPoolRef threads = NBThreadsPool_alloc(NULL);
        STNBIOPollstersPoolRef pollsters = NBIOPollstersPool_alloc(NULL);
        r = TRUE;
        //stopFlag
        if(r){
            NBStopFlag_setParentFlag(opq->stopFlag, parentStopFlag);
        }
        //threads
        if(r){
            //prepare
            if(!NBThreadsPool_setCfg(threads, &cfg->threads)){
                PRINTF_ERROR("TWContext, NBThreadsPool_setCfg failed.\n");
                r = FALSE;
            } else if(!NBThreadsPool_setParentStopFlag(threads, &opq->stopFlag)){
                PRINTF_ERROR("TWContext, NBThreadsPool_setParentStopFlag failed.\n");
                r = FALSE;
            } else if(!NBThreadsPool_prepare(threads)){
                PRINTF_ERROR("TWContext, NBThreadsPool_prepare failed.\n");
                r = FALSE;
            } else {
                PRINTF_INFO("TWContext, NBThreadsPool_prepare success.\n");
                //consume
                NBThreadsPool_set(&opq->threads.pool, &threads);
                NBThreadsPool_null(&threads);
            }
        }
        //pollsters
        if(r){
            //prepare
            if(!NBIOPollstersPool_setThreadsPool(pollsters, threads)){
                PRINTF_ERROR("TWContext, NBIOPollstersPool_prepare failed.\n");
                r = FALSE;
            } else if(!NBIOPollstersPool_setParentStopFlag(pollsters, &opq->stopFlag)){
                PRINTF_ERROR("TWContext, NBIOPollstersPool_setParentStopFlag failed.\n");
                r = FALSE;
            } else if(!NBIOPollstersPool_prepare(pollsters, &cfg->pollsters)){
                PRINTF_ERROR("TWContext, NBIOPollstersPool_prepare failed.\n");
                r = FALSE;
            } else {
                PRINTF_INFO("TWContext, NBIOPollstersPool_prepare success.\n");
                //consume
                NBIOPollstersPool_set(&opq->pollsters.pool, &pollsters);
                NBIOPollstersPool_null(&pollsters);
            }
        }
        //copy cfg
        if(r){
            NBStruct_stRelease(TWCfgContext_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
            if(cfg != NULL){
                NBStruct_stClone(TWCfgContext_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
            }
        }
        //release (if not consumed)
        if(NBThreadsPool_isSet(threads)){
            NBThreadsPool_release(&threads);
            NBThreadsPool_null(&threads);
        }
        if(NBIOPollstersPool_isSet(pollsters)){
            NBIOPollstersPool_release(&pollsters);
            NBIOPollstersPool_null(&pollsters);
        }
    }
    NBObject_unlock(opq);
    return r;
}

BOOL TWContext_start(STTWContextRef ref){
    BOOL r = FALSE;
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    if(NBThreadsPool_isSet(opq->threads.pool) && NBIOPollstersPool_isSet(opq->pollsters.pool) && !NBIOPollstersPool_isBussy(opq->pollsters.pool)){
        r = TRUE;
        //threads
        if(r){
            //started
        }
        //pollsters
        if(r){
            //prepare
            if(!NBIOPollstersPool_startThreads(opq->pollsters.pool)){
                PRINTF_ERROR("TWContext, NBIOPollstersPool_startThreads failed.\n");
                r = FALSE;
            } else {
                PRINTF_INFO("TWContext, NBIOPollstersPool_startThreads success.\n");
            }
        }
    }
    NBObject_unlock(opq);
    return r;
}

void TWContext_stopFlag(STTWContextRef ref){
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    {
        NBStopFlag_activate(opq->stopFlag);
    }
    NBObject_unlock(opq);
}

void TWContext_waitForAll(STTWContextRef ref){
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    while(
          (NBThreadsPool_isSet(opq->threads.pool) && NBThreadsPool_isBussy(opq->threads.pool))
          || (NBIOPollstersPool_isSet(opq->pollsters.pool) && NBIOPollstersPool_isBussy(opq->pollsters.pool))
          ){
        NBObject_unlock(opq);
        {
            NBThread_mSleep(100);
        }
        NBObject_lock(opq);
    }
    NBObject_unlock(opq);
}

//

BOOL TWContext_getStopFlag(STTWContextRef ref, STNBStopFlagRef* dst){
    BOOL r = FALSE;
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    if(NBStopFlag_isSet(opq->stopFlag)){
        if(dst != NULL){
            NBStopFlag_set(dst, &opq->stopFlag);
        }
        r = TRUE;
    }
    NBObject_unlock(opq);
    return r;
}

BOOL TWContext_getThreadsPool(STTWContextRef ref, STNBThreadsPoolRef* dst){
    BOOL r = FALSE;
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    if(NBThreadsPool_isSet(opq->threads.pool)){
        if(dst != NULL){
            NBThreadsPool_set(dst, &opq->threads.pool);
        }
        r = TRUE;
    }
    NBObject_unlock(opq);
    return r;
}

BOOL TWContext_getPollstersPool(STTWContextRef ref, STNBIOPollstersPoolRef* dst){
    BOOL r = FALSE;
    STTWContextOpq* opq = (STTWContextOpq*)ref.opaque; NBASSERT(TWContext_isClass(ref))
    NBObject_lock(opq);
    if(NBIOPollstersPool_isSet(opq->pollsters.pool)){
        if(dst != NULL){
            NBIOPollstersPool_set(dst, &opq->pollsters.pool);
        }
        r = TRUE;
    }
    NBObject_unlock(opq);
    return r;
}
