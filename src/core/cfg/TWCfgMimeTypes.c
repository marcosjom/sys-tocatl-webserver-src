//
//  TWCfgMimeTypes.c
//  tocatl-core
//
//  Created by Marcos Ortega on 15/9/25.
//

#include "nb/NBFrameworkPch.h"
#include "core/cfg/TWCfgMimeTypes.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//TWCfgMimeType

STNBStructMapsRec TWCfgMimeType_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgMimeType_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgMimeType_sharedStructMap);
    if(TWCfgMimeType_sharedStructMap.map == NULL){
        STTWCfgMimeType s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgMimeType);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addStrPtrM(map, s, ext);    //extension (including dot, like ".php" or ".")
        NBStructMap_addStrPtrM(map, s, mime);   //mime-type (like "application/octet-stream")
        //
        TWCfgMimeType_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgMimeType_sharedStructMap);
    return TWCfgMimeType_sharedStructMap.map;
}

//TWCfgMimeTypes

STNBStructMapsRec TWCfgMimeTypes_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TWCfgMimeTypes_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TWCfgMimeTypes_sharedStructMap);
    if(TWCfgMimeTypes_sharedStructMap.map == NULL){
        STTWCfgMimeTypes s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTWCfgMimeTypes);
        NBStructMap_init(map, sizeof(s));
        //members
        NBStructMap_addBoolM(map, s, ignoreDefaults); //do not use default well-known MIME types
        NBStructMap_addPtrToArrayOfStructM(map, s, types, typesSz, ENNBStructMapSign_Unsigned, TWCfgMimeType_getSharedStructMap()); //custom types, priorized before defaults mime-types
        //
        TWCfgMimeTypes_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TWCfgMimeTypes_sharedStructMap);
    return TWCfgMimeTypes_sharedStructMap.map;
}

const STTWCfgMimeType* TWCfgMimeTypes_getTypeByExt(const STTWCfgMimeTypes* types, const char* extWithDot){
    const STTWCfgMimeType* r = NULL;
    if(types != NULL && types->typesSz > 0 && types->types != NULL && extWithDot != NULL && extWithDot[0] != '\0'){
        const STTWCfgMimeType* t = types->types;
        const STTWCfgMimeType* tAfterEnd = t + types->typesSz;
        while(t < tAfterEnd){
            if(NBString_strIsLike(t->ext, extWithDot)){
                r = t;
                break;
            }
            ++t;
        }
    }
    return r;
}
