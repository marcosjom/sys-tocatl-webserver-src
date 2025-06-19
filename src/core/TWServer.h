//
//  TWServer.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#ifndef TWServer_h
#define TWServer_h

#include "nb/NBFrameworkDefs.h"
#include "core/cfg/TWCfg.h"
#include "core/TWContext.h"
//
#include "nb/NBObject.h"

#ifdef __cplusplus
extern "C" {
#endif

//TWServer

NB_OBJREF_HEADER(TWServer)

//

BOOL TWServer_prepare(STTWServerRef ref, const STTWCfg* cfg, STTWContextRef* ctx);
BOOL TWServer_start(STTWServerRef ref); //async
BOOL TWServer_execute(STTWServerRef ref, const UI64 msToRun); //blocking
void TWServer_stopFlag(STTWServerRef ref);      //stop-flag method
void TWServer_waitForAll(STTWServerRef ref);    //stop

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWServer_h */
