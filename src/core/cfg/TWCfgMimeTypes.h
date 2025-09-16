//
//  TWCfgMimeTypes.h
//  tocatl-core
//
//  Created by Marcos Ortega on 15/9/25.
//

#ifndef TWCfgMimeTypes_h
#define TWCfgMimeTypes_h

#include "nb/core/NBStructMap.h"

#ifdef __cplusplus
extern "C" {
#endif

//STTWCfgMimeType

typedef struct STTWCfgMimeType_ {
    char* ext;  //extension (without dot, like "php" or "zip")
    char* mime; //mime-type (like "application/octet-stream")
} STTWCfgMimeType;
    
const STNBStructMap* TWCfgMimeType_getSharedStructMap(void);

//STTWCfgMimeTypes

typedef struct STTWCfgMimeTypes_ {
    BOOL                ignoreDefaults; //do not use default well-known MIME types
    STTWCfgMimeType*    types;          //custom types, priorized before defaults mime-types
    UI32                typesSz;
} STTWCfgMimeTypes;
    
const STNBStructMap* TWCfgMimeTypes_getSharedStructMap(void);

const STTWCfgMimeType* TWCfgMimeTypes_getTypeByExt(const STTWCfgMimeTypes* types, const char* extNoDot);

#ifdef __cplusplus
} //extern "C"
#endif


#endif /* TWCfgMimeTypes_h */
