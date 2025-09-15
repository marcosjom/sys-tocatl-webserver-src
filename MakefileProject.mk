
#-------------------------
# PROJECT
#-------------------------

$(eval $(call nbCall,nbInitProject))

NB_PROJECT_NAME             := tocatl

NB_PROJECT_CFLAGS           := -fPIC

NB_PROJECT_CXXFLAGS         := -fPIC -std=c++11

NB_PROJECT_INCLUDES         := \
   src \
   ../../sys-nbframework/sys-nbframework-src/include \

#-------------------------
# TARGET
#-------------------------

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := tocatl-core

NB_TARGET_PREFIX            := lib

NB_TARGET_SUFIX             := .a

NB_TARGET_TYPE              := static

NB_TARGET_DEPENDS           := nbframework

NB_TARGET_FLAGS_ENABLES     += NB_LIB_TOCATL NB_LIB_SSL

#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := core

NB_CODE_GRP_FLAGS_REQUIRED  += NB_LIB_TOCATL

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += 

NB_CODE_GRP_SRCS            := \
    src/core/TWServer.c \
    src/core/TWContext.c \
    src/core/base/TWTimeframes.c \
    src/core/base/TWStrs.c \
    src/core/base/TWMimeTypesDefaults.c \
    src/core/stats/TWStatsTraffic.c \
    src/core/cfg/TWCfgStats.c \
    src/core/cfg/TWCfg.c \
    src/core/cfg/TWCfgWeb.c \
    src/core/reports/TWReport.c \
    src/core/reports/TWReportBldr.c \
    src/core/cfg/TWCfgContext.c


$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))


#-------------------------
# TARGET
#-------------------------

#Specific OS files
ifeq (,$(findstring Android,$(NB_CFG_HOST)))

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := tocatl

NB_TARGET_PREFIX            :=

NB_TARGET_SUFIX             :=

NB_TARGET_TYPE              := exe

NB_TARGET_DEPENDS           := tocatl-core

NB_TARGET_FLAGS_ENABLES     += NB_LIB_TOCATL

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
else
ifeq ($(OS),Windows_NT)
  #Windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    #Linux
  endif
  ifeq ($(UNAME_S),Darwin)
    #OSX
    NB_TARGET_FRAMEWORKS    += Foundation Security
  endif
endif
endif


#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := all

NB_CODE_GRP_FLAGS_REQUIRED  +=

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_TOCATL

NB_CODE_GRP_SRCS            := \
   src/tocatl/main.c \

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))

endif










#-------------------------
# TARGET
#-------------------------

#Specific OS files
ifeq (,$(findstring Android,$(NB_CFG_HOST)))

$(eval $(call nbCall,nbInitTarget))

NB_TARGET_NAME              := tocatl-test

NB_TARGET_PREFIX            :=

NB_TARGET_SUFIX             :=

NB_TARGET_TYPE              := exe

NB_TARGET_DEPENDS           := tocatl-core

NB_TARGET_FLAGS_ENABLES     += NB_LIB_TOCATL

#Specific OS
ifneq (,$(findstring Android,$(NB_CFG_HOST)))
  #Android
else
ifeq ($(OS),Windows_NT)
  #Windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    #Linux
  endif
  ifeq ($(UNAME_S),Darwin)
    #OSX
    NB_TARGET_FRAMEWORKS    += Foundation Security
  endif
endif
endif


#-------------------------
# CODE GRP
#-------------------------

$(eval $(call nbCall,nbInitCodeGrp))

NB_CODE_GRP_NAME            := all

NB_CODE_GRP_FLAGS_REQUIRED  +=

NB_CODE_GRP_FLAGS_FORBIDDEN +=

NB_CODE_GRP_FLAGS_ENABLES   += NB_LIB_TOCATL

NB_CODE_GRP_SRCS            := \
   src/test/main.c \

$(eval $(call nbCall,nbBuildCodeGrpRules))

#-------------------------
# TARGET RULES
#-------------------------

$(eval $(call nbCall,nbBuildTargetRules))

endif







#-------------------------
# PROJECT RULES
#-------------------------

$(eval $(call nbCall,nbBuildProjectRules))
