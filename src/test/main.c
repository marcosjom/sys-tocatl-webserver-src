//
//  main.c
//  tocatl-webserver-test
//
//  Created by Marcos Ortega on 17/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "nb/NBFrameworkDefs.h"
//
#include <stdlib.h>	//for rand()
#include <time.h>	//for time()
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBMngrProcess.h"
#include "nb/net/NBSocket.h"
//
#include "nb/core/NBIOPollster.h"
#include "nb/core/NBNumParser.h"
#include "nb/net/NBSocket.h"
#include "nb/net/NBWebSocket.h"
#include "nb/net/NBStompFrame.h"
#include "nb/net/NBHttpBuilder.h"
#include "nb/crypto/NBBase64.h"
#include "nb/ssl/NBSslContext.h"

//main
int main(int argc, const char * argv[]) {
	//init-nb-engine
	NBMngrProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	srand((int)time(NULL)); //start-randomizer
	//execute
	{
		//
	}
	//end-nb-engine
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBMngrProcess_release();
	//end
	printf("end-of-main.\n");
	return 0;
}
