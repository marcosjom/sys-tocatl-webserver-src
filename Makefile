
LOCAL_PATH := $(call my-dir)

#Configure
NB_CFG_PRINT_INTERNALS := 0
NB_CFG_PRINT_INFO      := 0

#Import functions
include ../../../CltNicaraguaBinary/sys-nbframework/lib-nbframework-src/MakefileFuncs.mk

#Init workspace
$(eval $(call nbCall,nbInitWorkspace))

#Import projects
include ../../../CltNicaraguaBinary/sys-nbframework/lib-nbframework-src/MakefileProject.mk

#Project
include MakefileProject.mk

#Build workspace
$(eval $(call nbCall,nbBuildWorkspaceRules))

#Install rule
install:
	cp $(NB_WORKSPACE_OUT_DIR_BIN)/tocatl /bin
	cp sys-tocatl-webserver-res/tocatl.service.cfg.json /etc
	cp sys-tocatl-webserver-res/tocatl.service.cfg /usr/lib/systemd/system/tocatl.service
	systemctl daemon-reload

#Clean rule
clean:
	rm -r bin
	rm -r tmp

