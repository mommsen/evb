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
	CRC16.cc \
	DumpUtility.cc \
	EvBidFactory.cc \
	FragmentGenerator.cc \
	FragmentTracker.cc \
	I2OMessages.cc \
	InfoSpaceItems.cc \
	EVM.cc \
	evm/FEROLproxy.cc \
	evm/RUproxy.cc \
	RU.cc \
	ru/FEROLproxy.cc \
	BU.cc \
	bu/DiskUsage.cc \
	bu/DiskWriter.cc \
	bu/Event.cc \
	bu/EventBuilder.cc \
	bu/FileHandler.cc \
	bu/ResourceManager.cc \
	bu/RUproxy.cc \
	bu/StateMachine.cc \
	bu/StreamHandler.cc \
	version.cc

TestSources = \
	DummyFEROL.cc \
	dummyFEROL/StateMachine.cc

TestExecutables = \
	OneToOneQueue.cxx

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

TestIncludeDirs = test/include

TestLibraries = \
	b2innub \
	boost_system \
	boost_thread-mt \
	cgicc \
	config \
	evb \
	executive \
	i2o \
	i2outils \
	log4cplus \
	logudpappender \
	logxmlappender \
	mimetic \
	numa \
	peer \
	pttcp \
	ptutcp \
	tcpla \
	toolbox \
	asyncresolv \
	uuid \
	xcept \
	xdaq \
	xdata \
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
        $(I2O_LIB_PREFIX)  \
        $(I2O_UTILS_LIB_PREFIX)  \
        $(LOG4CPLUS_LIB_PREFIX)  \
        $(LOGUDPAPPENDER_LIB_PREFIX)  \
        $(LOGXMLAPPENDER_LIB_PREFIX)  \
        $(MIMETIC_LIB_PREFIX) \
        $(PEER_LIB_PREFIX) \
        $(PT_LIB_PREFIX) \
        $(TOOLBOX_LIB_PREFIX) \
        $(UUID_LIB_PREFIX) \
        $(XCEPT_LIB_PREFIX) \
        $(XDAQ_LIB_PREFIX) \
        $(XDATA_LIB_PREFIX) \
        $(XERCES_LIB_PREFIX) \
        $(XGI_LIB_PREFIX) \
        $(XOAP_LIB_PREFIX)

UserCCFlags = -O3 -funroll-loops -Wno-long-long -Werror -fno-omit-frame-pointer

# These libraries can be platform specific and
# potentially need conditional processing
DependentLibraries = interfaceshared xdaq2rc boost_filesystem boost_thread-mt boost_system
DependentLibraryDirs += /usr/lib64 $(INTERFACE_SHARED_LIB_PREFIX)
UserDynamicLinkFlags = src/$(XDAQ_OS)/$(XDAQ_PLATFORM)/crc16_T10DIF_128x_extended.o

#
# Compile the source files and create a shared library
#
DynamicLibrary=evb
TestDynamicLibrary=evbtest

TestDynamicLibraryName = $(TestDynamicLibrary:%=$(PackageLibDir)/$(LibraryPrefix)%$(DynamicSuffix))

all: asm _buildall test install

test: _buildall
	@make tests

testinstall: test
	$(Copy) -p $(TestDynamicLibraryName)* $(LibInstallDir)/

clean: _cleanall
	@make testsclean
	@rm -f $(PackageTargetDir)/crc16_T10DIF_128x_extended.o

install: _installall testinstall

rpm: _buildall test _rpmall

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules

asm: src/common/crc16_T10DIF_128x_extended.S
	gcc -c -o $(PackageTargetDir)/crc16_T10DIF_128x_extended.o $<
