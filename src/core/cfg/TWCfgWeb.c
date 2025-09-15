//
//  TWCfgWeb.c
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#include "nb/NBFrameworkPch.h"
#include "core/cfg/TWCfgWeb.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//TWCfgWebPathRuleChars

STNBStructMapsRec TWCfgWebPathRuleChars_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgWebPathRuleChars_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgWebPathRuleChars_sharedStructMap);
    if(TWCfgWebPathRuleChars_sharedStructMap.map == NULL){
        STTWCfgWebPathRuleChars s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgWebPathRuleChars);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addBoolM(map, s, disableDefaults); //use defaults-chars (if forbidden, these are [^], ["], [`], ['] blacklisted)
        NBStructMap_addStrPtrM(map, s, list); //list (NULL is default)
        //
        TWCfgWebPathRuleChars_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgWebPathRuleChars_sharedStructMap);
    return TWCfgWebPathRuleChars_sharedStructMap.map;
}

//TWCfgWebPathChars

STNBStructMapsRec TWCfgWebPathChars_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgWebPathChars_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgWebPathChars_sharedStructMap);
    if(TWCfgWebPathChars_sharedStructMap.map == NULL){
        STTWCfgWebPathChars s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgWebPathChars);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addStructPtrM(map, s, black, TWCfgWebPathRuleChars_getSharedStructMap());
        NBStructMap_addStructPtrM(map, s, white, TWCfgWebPathRuleChars_getSharedStructMap());
        //
        TWCfgWebPathChars_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgWebPathChars_sharedStructMap);
    return TWCfgWebPathChars_sharedStructMap.map;
}

//TWCfgWebPath

STNBStructMapsRec TWCfgWebPath_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgWebPath_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgWebPath_sharedStructMap);
    if(TWCfgWebPath_sharedStructMap.map == NULL){
        STTWCfgWebPath s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgWebPath);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addStrPtrM(map, s, root); //website root-path (final slash will be automatically removed, empty root is automatically replaced to ".")
        NBStructMap_addStructPtrM(map, s, chars, TWCfgWebPathChars_getSharedStructMap());
        NBStructMap_addStructPtrM(map, s, mimeTypes, TWCfgMimeTypes_getSharedStructMap());
        NBStructMap_addPtrToArrayOfStrPtrM(map, s, defaultDocs, defaultDocsSz, ENNBStructMapSign_Unsigned);
        NBStructMap_addBoolM(map, s, describeFolders); //if TRUE, when a folder path was not served, return the content/files of the folder (security risk).
        //
        TWCfgWebPath_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgWebPath_sharedStructMap);
    return TWCfgWebPath_sharedStructMap.map;
}

//TWCfgHostPort

STNBStructMapsRec TWCfgHostPort_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgHostPort_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgHostPort_sharedStructMap);
    if(TWCfgHostPort_sharedStructMap.map == NULL){
        STTWCfgHostPort s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgHostPort);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addStrPtrM(map, s, name);   //hostname ('*' means 'all')
        NBStructMap_addUIntM(map, s, port);     //port of the server ('0' means 'all')
        //
        TWCfgHostPort_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgHostPort_sharedStructMap);
    return TWCfgHostPort_sharedStructMap.map;
}

//TWCfgWebSite

STNBStructMapsRec TWCfgWebSite_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgWebSite_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgWebSite_sharedStructMap);
    if(TWCfgWebSite_sharedStructMap.map == NULL){
        STTWCfgWebSite s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgWebSite);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addPtrToArrayOfStructM(map, s, hostnames, hostnamesSz, ENNBStructMapSign_Unsigned, TWCfgHostPort_getSharedStructMap());
        NBStructMap_addStructPtrM(map, s, path, TWCfgWebPath_getSharedStructMap());
        //
        TWCfgWebSite_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgWebSite_sharedStructMap);
    return TWCfgWebSite_sharedStructMap.map;
}

//TWCfgWeb

STNBStructMapsRec TWCfgWeb_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgWeb_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TWCfgWeb_sharedStructMap);
	if(TWCfgWeb_sharedStructMap.map == NULL){
		STTWCfgWeb s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgWeb);
		NBStructMap_init(map, sizeof(s));
		//members
        NBStructMap_addStructPtrM(map, s, defaults, TWCfgWebPath_getSharedStructMap()); //defaults (individual) values for sites
        NBStructMap_addPtrToArrayOfStructM(map, s, sites, sitesSz, ENNBStructMapSign_Unsigned, TWCfgWebSite_getSharedStructMap());
		//
		TWCfgWeb_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TWCfgWeb_sharedStructMap);
	return TWCfgWeb_sharedStructMap.map;
}
