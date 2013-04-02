# $Id: Makefile,v 1.9 2009/02/04 15:17:18 cschwick Exp $

#########################################################################
# XDAQ Components for Distributed Data Acquisition                      #
# Copyright (C) 2000-2004, CERN.			                #
# All rights reserved.                                                  #
# Authors: R.K. Mommsen                                                 #
#                                                                       #
# For the licensing terms see LICENSE.		                        #
# For the list of contributors see CREDITS.   			        #
#########################################################################

##
#
# This is the TriDAS/daq/evb Package Makefile
#
##

BUILD_HOME:=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)
include $(XDAQ_ROOT)/config/mfDefs.extern_coretools
include $(XDAQ_ROOT)/config/mfDefs.coretools
include $(XDAQ_ROOT)/config/mfDefs.general_worksuite

#
# Packages to be built
#
Project=daq
Package=evb

Sources=\
	BU.cc \
	bu/DiskUsage.cc \
	bu/DiskWriter.cc \
	bu/Event.cc \
	bu/EventTable.cc \
	bu/FUproxy.cc \
	bu/FileHandler.cc \
	bu/LumiHandler.cc \
	bu/RUbroadcaster.cc \
	bu/RUproxy.cc \
	bu/StateMachine.cc \
	DumpUtility.cc \
	EvBidFactory.cc \
	EventUtils.cc \
	I2OMessages.cc \
	InfoSpaceItems.cc \
	RU.cc \
	SuperFragmentGenerator.cc \
	SuperFragmentTracker.cc \
	TimerManager.cc \
	ru/BUproxy.cc \
	ru/DummyInputData.cc \
	ru/FEROLproxy.cc \
	ru/Input.cc \
	ru/StateMachine.cc \
	ru/SuperFragment.cc \
	ru/SuperFragmentTable.cc \
	version.cc

IncludeDirs = \
	$(XERCES_INCLUDE_PREFIX) \
	$(LOG4CPLUS_INCLUDE_PREFIX) \
	$(ASYNCRESOLV_INCLUDE_PREFIX) \
	$(CGICC_INCLUDE_PREFIX) \
	$(CONFIG_INCLUDE_PREFIX) \
	$(XCEPT_INCLUDE_PREFIX) \
	$(LOG_UDPAPPENDER_INCLUDE_PREFIX) \
	$(LOG_XMLAPPENDER_INCLUDE_PREFIX) \
	$(XOAP_INCLUDE_PREFIX) \
	$(XGI_INCLUDE_PREFIX) \
	$(XGI_TOOLS_INCLUDE_PREFIX) \
	$(XDATA_INCLUDE_PREFIX) \
	$(TOOLBOX_INCLUDE_PREFIX) \
	$(PT_INCLUDE_PREFIX) \
	$(XDAQ_INCLUDE_PREFIX) \
	$(I2O_INCLUDE_PREFIX) \
	$(XI2O_UTILS_INCLUDE_PREFIX) \
	$(XI2O_INCLUDE_PREFIX) \
	$(XDAQ2RC_INCLUDE_PREFIX) \
	$(INTERFACE_EVB_INCLUDE_PREFIX) \
	$(INTERFACE_SHARED_INCLUDE_PREFIX)

UserCCFlags = -O3 -pedantic-errors -Wno-long-long -Werror 

# These libraries can be platform specific and
# potentially need conditional processing
Libraries = boost_thread
DependentLibraryDirs += /usr/lib

#
# Compile the source files and create a shared library
#
DynamicLibrary=evb

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
