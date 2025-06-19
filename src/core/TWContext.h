//
//  TWContext.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 29/11/21.
//

#ifndef TWContext_h
#define TWContext_h

#include "nb/NBFrameworkDefs.h"
//
#include "nb/NBObject.h"
#include "nb/core/NBStopFlag.h"
#include "nb/core/NBThreadsPool.h"
#include "nb/core/NBIOPollstersPool.h"
//
#include "core/cfg/TWCfgContext.h"

#ifdef __cplusplus
extern "C" {
#endif

//TWContext

NB_OBJREF_HEADER(TWContext)

//

BOOL TWContext_prepare(STTWContextRef ref, const STTWCfgContext* cfg, STNBStopFlagRef* parentStopFlag);
BOOL TWContext_start(STTWContextRef ref);
void TWContext_stopFlag(STTWContextRef ref);
void TWContext_waitForAll(STTWContextRef ref);

BOOL TWContext_getStopFlag(STTWContextRef ref, STNBStopFlagRef* dst);
BOOL TWContext_getThreadsPool(STTWContextRef ref, STNBThreadsPoolRef* dst);
BOOL TWContext_getPollstersPool(STTWContextRef ref, STNBIOPollstersPoolRef* dst);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWContext_h */
