//
//  main.c
//  tocatl-webserver
//
//  Created by Marcos Ortega on 25/11/21.
//

#if defined(_WIN32) || defined(_WIN64)
#	define _CRT_SECURE_NO_WARNINGS
//#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>		//for CRITICAL_SECTION
#	include <stdio.h>
#endif

#include "nb/NBFrameworkPch.h"
#include "nb/NBFrameworkDefs.h"
//
#include <stdlib.h>	//for rand()
#include <time.h>	//for time()
#include "nb/core/NBStopFlag.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBMngrProcess.h"
#include "nb/net/NBSocket.h"
//
#include "nb/core/NBStruct.h"
#include "core/cfg/TWCfg.h"
#include "core/TWContext.h"
#include "core/TWServer.h"
#if defined(__APPLE__)
#	include <unistd.h>	//for 'getcwd'
#endif
#if !defined(_WIN32) && !defined(_WIN64)
#	include <signal.h>	//for interrupt-signals
#endif

//

STNBStopFlagRef __stopFlag;
STTWContextRef __context;
STTWServerRef __server;

//Interruptions defs

#if !defined(_WIN32) && !defined(_WIN64)
typedef enum ENSignalAction_ {
	ENSignalAction_Ignore = 0,
	ENSignalAction_GracefullExit,
	ENSignalAction_Count
} ENSignalAction;
#endif

#if !defined(_WIN32) && !defined(_WIN64)
typedef struct STSignalDef_ {
	int				sig;
	const char*		sigName;
	ENSignalAction	action;
} STSignalDef;
#endif

#if !defined(_WIN32) && !defined(_WIN64)
static STSignalDef _signalsDefs[] = {
	//sockets SIGPIPE signals (for unix-like systems)
	{ SIGPIPE, "SIGPIPE", ENSignalAction_Ignore}, //Ignore
	//termination signals: https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html
	{ SIGTERM, "SIGTERM", ENSignalAction_GracefullExit },
	{ SIGINT, "SIGINT", ENSignalAction_GracefullExit },
	{ SIGQUIT, "SIGQUIT", ENSignalAction_GracefullExit },
	{ SIGKILL, "SIGKILL", ENSignalAction_GracefullExit },
	{ SIGHUP, "SIGHUP", ENSignalAction_GracefullExit },
};
#endif

#if !defined(_WIN32) && !defined(_WIN64)
void TWMain_interruptHandler(int sig){
	//Note: interruptions are called without considerations
	//of mutexes and threads states. Avoid non interrupt-safe methods calls.
	SI32 i; const SI32 count = (sizeof(_signalsDefs) / sizeof(_signalsDefs[0]));
	for(i = 0; i < count; i++){
		const STSignalDef* def = &_signalsDefs[i];
		if(sig == def->sig){
			if(def->action == ENSignalAction_GracefullExit){
                if(NBStopFlag_isSet(__stopFlag)){
                    NBStopFlag_activate(__stopFlag);
                }
			}
			break;
		}
	}
}
#endif

//main

int main(int argc, const char * argv[]) {
	const char* cfgPath = NULL;
	BOOL runServer = FALSE;
	UI32 msSleepBeforeRun = 0, msSleepAfterRun = 0, msRunAndQuit = 0;
	//
	__stopFlag	= NB_OBJREF_NULL;
	__context	= NB_OBJREF_NULL;
	__server	= NB_OBJREF_NULL;
	//init-nb-engine
	NBMngrProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	srand((int)time(NULL)); //start-randomizer
	//Apply signal handlers.
	//Ignore SIGPIPE at process level (for unix-like systems)
#	if !defined(_WIN32) && !defined(_WIN64)
	{
		SI32 i; const SI32 count = (sizeof(_signalsDefs) / sizeof(_signalsDefs[0]));
		for(i = 0; i < count; i++){
			const STSignalDef* def = &_signalsDefs[i];
			if(def->action == ENSignalAction_Ignore){
				struct sigaction act;
				act.sa_handler	= SIG_IGN;
				act.sa_flags	= 0;
				sigemptyset(&act.sa_mask);
				sigaction(def->sig, &act, NULL);
			} else if(def->action == ENSignalAction_GracefullExit){
				struct sigaction act;
				act.sa_handler	= TWMain_interruptHandler;
				act.sa_flags	= 0;
				sigemptyset(&act.sa_mask);
				sigaction(def->sig, &act, NULL);
			}
		}
	}
#	endif
	//Load params
	{
		SI32 i; for(i = 0; i < argc; i++){
			if(NBString_strIsEqual(argv[i], "-c")){
				if((i + 1) < argc){
					cfgPath = argv[++i];
				}
			} else if(NBString_strIsEqual(argv[i], "-msSleepBeforeRun")){
				if((i + 1) < argc){
					if(NBString_strIsInteger(argv[i + 1])){
						msSleepBeforeRun = NBString_strToSI32(argv[++i]); 
					}
				}
			} else if(NBString_strIsEqual(argv[i], "-msSleepAfterRun")){
				if((i + 1) < argc){
					if(NBString_strIsInteger(argv[i + 1])){
						msSleepAfterRun = NBString_strToSI32(argv[++i]); 
					}
				}
			} else if(NBString_strIsEqual(argv[i], "-msRunAndQuit")){
				if((i + 1) < argc){
					if(NBString_strIsInteger(argv[i + 1])){
						msRunAndQuit = NBString_strToSI32(argv[++i]);
					}
				}
			} else if(NBString_strIsEqual(argv[i], "-runServer")){
				runServer = TRUE;
			}
		}
		if(msRunAndQuit > 0){
			PRINTF_INFO("-----\n");
			PRINTF_INFO("- Run will be limited to %dms.\n", msRunAndQuit);
			PRINTF_INFO("-----\n");
		}
		if(msSleepBeforeRun > 0){
			PRINTF_INFO("-----\n");
			PRINTF_INFO("Sleeping %dms before running...\n", msSleepBeforeRun);
			PRINTF_INFO("-----\n");
			NBThread_mSleep(msSleepBeforeRun);
		}
	}
#	if defined(__APPLE__) && defined(NB_CONFIG_INCLUDE_ASSERTS)
	{
		//Requires #include <unistd.h>
		char path[4096];
		getcwd(path, sizeof(path));
		PRINTF_INFO("-----\n");
		PRINTF_INFO("Current working directory: '%s'.\n", path);
		PRINTF_INFO("-----\n");
	}
#	endif
	{
		volatile UI32 i = 0x01234567;
		const BOOL isLittleEndian = (*((BYTE*)(&i))) == 0x67;
		PRINTF_INFO("System is %s-endian.\n", isLittleEndian ? "little" : "BIG");
	}
	//Execute
	{
		if(cfgPath == NULL){
			PRINTF_CONSOLE_ERROR("MAIN, must specify a config file: -c filepath.\n");
		} else if(cfgPath[0] == '\0'){
			PRINTF_CONSOLE_ERROR("MAIN, must specify a non-empty config file: -c filepath.\n");
		} else {
			const STNBStructMap* cfgMap = TWCfg_getSharedStructMap();
			STTWCfg	cfg;
			NBMemory_setZeroSt(cfg, STTWCfg);
			if(!NBStruct_stReadFromJsonFilepath(cfgPath, cfgMap, &cfg, sizeof(cfg))){
				PRINTF_CONSOLE_ERROR("MAIN, could not load config file: '%s'.\n", cfgPath);
			} else {
				PRINTF_INFO("MAIN, config file loaded: '%s'.\n", cfgPath);
                {
                    __stopFlag = NBStopFlag_alloc(NULL);
                    __context = TWContext_alloc(NULL);
                    if(!TWContext_prepare(__context, &cfg.context, &__stopFlag)){
                        PRINTF_CONSOLE_ERROR("MAIN, TWContext_prepare failed with: '%s'.\n", cfgPath);
                    } else if(!TWContext_start(__context)){
                        PRINTF_CONSOLE_ERROR("MAIN, TWContext_start failed with: '%s'.\n", cfgPath);
                    } else {
                        PRINTF_INFO("MAIN, TWContext_start success with: '%s'.\n", cfgPath);
                        __server = TWServer_alloc(NULL);
                        if(!TWServer_prepare(__server, &cfg, &__context)){
                            PRINTF_CONSOLE_ERROR("MAIN, TWServer_prepare failed with: '%s'.\n", cfgPath);
                        } else if(!TWServer_execute(__server, msRunAndQuit)){
                            PRINTF_CONSOLE_ERROR("MAIN, could not execute server with config file: '%s'.\n", cfgPath);
                        } else {
                            PRINTF_INFO("MAIN, TWServer_execute success with: '%s'.\n", cfgPath);
                        }
                        if(TWServer_isSet(__server)){
                            TWServer_stopFlag(__server);
                            TWServer_waitForAll(__server);
                            TWServer_release(&__server);
                            TWServer_null(&__server);
                        }
                    }
                    if(TWContext_isSet(__context)){
                        TWContext_stopFlag(__context);
                        TWContext_waitForAll(__context);
                        TWContext_release(&__context);
                        TWContext_null(&__context);
                    }
                    if(NBStopFlag_isSet(__stopFlag)){
                        NBStopFlag_activate(__stopFlag);
                        NBStopFlag_release(&__stopFlag);
                        NBStopFlag_null(&__stopFlag);
                    }
                }
			}
			NBStruct_stRelease(cfgMap, &cfg, sizeof(cfg));
		}
	}
	//sleep-after
	if(msSleepAfterRun > 0){
		PRINTF_INFO("-----\n");
		PRINTF_INFO("... sleeping %dms after running.\n", msSleepAfterRun);
		PRINTF_INFO("-----\n");
	}
	//end-nb-engine
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBMngrProcess_release();
	//end
	printf("end-of-main.\n");
	return 0;
}




