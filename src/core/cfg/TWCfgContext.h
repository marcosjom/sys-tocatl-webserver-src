//
//  TWCfgContext.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#ifndef TWCfgContext_h
#define TWCfgContext_h

#include "nb/core/NBStructMap.h"

#include "nb/core/NBThreadsPool.h"
#include "nb/core/NBIOPollstersPool.h"

#ifdef __cplusplus
extern "C" {
#endif

//TWCfgContext

typedef struct STTWCfgContext_ {
    STNBThreadsPoolCfg      threads;    //threads-pool
    STNBIOPollstersPoolCfg  pollsters;  //pollsters
} STTWCfgContext;
	
const STNBStructMap* TWCfgContext_getSharedStructMap(void);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWCfgNet_h */
