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
include $(XDAQ_ROOT)/config/mfDefs.powerpack
include $(XDAQ_ROOT)/config/mfDefs.general_worksuite

#
# Packages to be built
#
Project=daq
Package=evb

Sources=\
	crc16_T10DIF_128x_extended.S \
	crc32c.cc \
	CRCCalculator.cc \
	DumpUtility.cc \
	EvBidFactory.cc \
	FragmentSize.cc \
	FragmentTracker.cc \
	I2OMessages.cc \
	InfoSpaceItems.cc \
	readoutunit/DummyFragment.cc \
	readoutunit/FedFragment.cc \
	readoutunit/SuperFragment.cc \
	EVM.cc \
	evm/RUproxy.cc \
	RU.cc \
	BU.cc \
	bu/DiskUsage.cc \
	bu/DiskWriter.cc \
	bu/Event.cc \
	bu/EventBuilder.cc \
	bu/EventInfo.cc \
	bu/FedInfo.cc \
	bu/FileHandler.cc \
	bu/FragmentChain.cc \
	bu/ResourceManager.cc \
	bu/RUproxy.cc \
	bu/StateMachine.cc \
	bu/StreamHandler.cc \
	version.cc \
	DummyFEROL.cc \
	dummyFEROL/FragmentGenerator.cc \
	dummyFEROL/StateMachine.cc

TestExecutables = \
	EvBid.cxx \
	Fibonacci.cxx \
	LogNormal.cxx \
	OneToOneQueue.cxx \
	OneToOneQueueWait.cxx

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
	$(PTBLIT_INCLUDE_PREFIX) \
	$(XDAQ_INCLUDE_PREFIX) \
	$(I2O_INCLUDE_PREFIX) \
	$(XI2O_UTILS_INCLUDE_PREFIX) \
	$(XI2O_INCLUDE_PREFIX) \
	$(XDAQ2RC_INCLUDE_PREFIX) \
        $(TCPLA_INCLUDE_PREFIX) \
	$(INTERFACE_EVB_INCLUDE_PREFIX) \
	$(INTERFACE_SHARED_INCLUDE_PREFIX)

TestIncludeDirs = test/include

TestLibraries = \
	b2innub \
	boost_system \
	boost_thread-mt \
	cgicc \
	config \
	evb \
	executive \
	interfaceshared \
	log4cplus \
	logudpappender \
	logxmlappender \
	mimetic \
	numa \
	peer \
	ptblit \
	tcpla \
	toolbox \
	asyncresolv \
	uuid \
	xcept \
	xdaq \
	xdata \
	xdaq2rc \
	xerces-c \
	xgi \
	xoap

TestLibraryDirs = \
        $(ASYNCRESOLV_LIB_PREFIX) \
        $(B2IN_NUB_LIB_PREFIX) \
        $(CGICC_LIB_PREFIX) \
        $(CONFIG_LIB_PREFIX) \
        $(EVB_LIB_PREFIX)  \
        $(EXECUTIVE_LIB_PREFIX)  \
        $(LOG4CPLUS_LIB_PREFIX)  \
        $(LOG_UDPAPPENDER_LIB_PREFIX) \
        $(LOG_XMLAPPENDER_LIB_PREFIX) \
        $(MIMETIC_LIB_PREFIX) \
        $(PEER_LIB_PREFIX) \
        $(PT_LIB_PREFIX) \
        $(PTBLIT_LIB_PREFIX) \
        $(TOOLBOX_LIB_PREFIX) \
        $(UUID_LIB_PREFIX) \
        $(XCEPT_LIB_PREFIX) \
        $(INTERFACE_SHARED_LIB_PREFIX) \
        $(XDAQ_LIB_PREFIX) \
        $(TCPLA_LIB_PREFIX) \
        $(XDATA_LIB_PREFIX) \
        $(XERCES_LIB_PREFIX) \
        $(XGI_LIB_PREFIX) \
        $(XDAQ2RC_LIB_PREFIX) \
        $(XOAP_LIB_PREFIX)

UserCCFlags = -O3 -funroll-loops -Werror #-std=c++0x
#UserCCFlags += -DEVB_DEBUG_CORRUPT_EVENT

# These libraries can be platform specific and
# potentially need conditional processing
DependentLibraries = interfaceshared xdaq2rc ptblit boost_regex boost_filesystem boost_thread-mt boost_system curl
DependentLibraryDirs += /usr/lib64 $(INTERFACE_SHARED_LIB_PREFIX) $(XDAQ2RC_LIB_PREFIX) $(PTBLIT_LIB_PREFIX)

#
# Compile the source files and create a shared library
#
DynamicLibrary=evb
TestDynamicLibrary=evbtest

TestDynamicLibraryName = $(TestDynamicLibrary:%=$(PackageLibDir)/$(LibraryPrefix)%$(DynamicSuffix))

all: _all test install

test: _all
	@make tests

testinstall: test
	$(Copy) -p $(TestDynamicLibraryName)* $(LibInstallDir)/

clean: _cleanall
	@make testsclean

install: _installall testinstall

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
