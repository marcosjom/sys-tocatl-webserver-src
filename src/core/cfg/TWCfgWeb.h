//
//  TWCfgWeb.h
//  tocatl-webserver-core
//
//  Created by Marcos Ortega on 25/11/21.
//

#ifndef TWCfgWeb_h
#define TWCfgWeb_h

#include "nb/core/NBStructMap.h"
#include "core/cfg/TWCfgWeb.h"
#include "core/cfg/TWCfgMimeTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


//TWCfgWebPathRules

typedef struct STTWCfgWebPathRuleChars_ {
    BOOL    disableDefaults;     //do not white/black-list chars defined in http-specs (def-blacklist: [^], ["], [`], ['], def-whitelist: none)
    char*   list;                //list (NULL is default)
} STTWCfgWebPathRuleChars;

const STNBStructMap* TWCfgWebPathRuleChars_getSharedStructMap(void);

//TWCfgWebPathRules

typedef struct STTWCfgWebPathChars_ {
    STTWCfgWebPathRuleChars*   black;
    STTWCfgWebPathRuleChars*   white;
} STTWCfgWebPathChars;

const STNBStructMap* TWCfgWebPathChars_getSharedStructMap(void);

//TWCfgWebPath

typedef struct STTWCfgWebPath_ {
    char*                   root;               //website root-path (final slash will be automatically removed, empty root is automatically replaced to ".")
    STTWCfgWebPathChars*    chars;
    STTWCfgMimeTypes*       mimeTypes;          //mime-types config
    char**                  defaultDocs;        //filenames to add if folder is provided (like "index.html" or "main.html")
    UI32                    defaultDocsSz;
    BOOL                    describeFolders;    //if TRUE, when a folder path was not served, return the content/files of the folder (security risk).
} STTWCfgWebPath;

const STNBStructMap* TWCfgWebPath_getSharedStructMap(void);

//TWCfgHostname

typedef struct STTWCfgHostPort_ {
    char*         name;     //hostname (like: '127.0.0.1', 'localhost' or 'my.host.com'; '*' means 'any')
    UI16          port;     //port of the server (like: '80' or '443'; '0' means 'all ports')
} STTWCfgHostPort;

const STNBStructMap* TWCfgHostPort_getSharedStructMap(void);

//TWCfgWebSite

typedef struct STTWCfgWebSite_ {
    STTWCfgHostPort*    hostnames;     //hostnames for this site (like: '127.0.0.1:80', 'localhost:443' or 'my.host.com:443'; '*:0' means 'any')
    UI32                hostnamesSz;
    STTWCfgWebPath*     path;
} STTWCfgWebSite;

const STNBStructMap* TWCfgWebSite_getSharedStructMap(void);

//TWCfgWeb

typedef struct STTWCfgWeb_ {
    STTWCfgWebPath*     defaults;   //defaults (individual) values for sites
    STTWCfgWebSite*     sites;      //host names to serve
    UI32                sitesSz;
} STTWCfgWeb;
	
const STNBStructMap* TWCfgWeb_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TWCfgWeb_h */
