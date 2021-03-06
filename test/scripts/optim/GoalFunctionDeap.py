#!/usr/bin/env python

import logging, time
import numpy as np

import sys, os
parent_path = os.path.join(os.path.abspath(os.path.dirname(__file__)), "..")
if not parent_path in sys.path:
    sys.path.append(parent_path)

class GoalFunctionDeap:
    # goal function to maximize by adjusting the assignment
    # of work loops to CPU cores

    #----------------------------------------
    # the figure of merit is the event rate (we assume constant
    # event size) which we read from the EVM
    #
    # this function applies the pinning of the processes as given
    # in the args variable to __call__() and collects 
    # event building information for some given time
    # and returns the average event rate (including variation)
    #----------------------------------------

    def __init__(self, config, workLoopList,
                 measurementPeriod = 10):
        # @param workLoopList must be an object of type WorkLoopList
        # which contains information about the process ids on each 
        # host of the work loops we want to tune
        #
        # @param measurementPeriod is the overall period
        # (in seconds) for which we should leave the threads 
        # pinned as given in the configuration and 
        # sample the EVM rate (for the moment this is 
        # sampled each second)
        #
        # @param config must be an instance of a Configuration object.
        # This is used to retrieve the application states e.g. when the 
        # readout rate drops to zero. It is also used to 
        # get information about the EVM application to 
        # retrieve the readout rate.
        
        # EVM host for reading the rate
        self.config = config
        evmApplication = self.config.applications['EVM'][0]
        self.evmHostPort = (evmApplication['soapHostname'], int(evmApplication['soapPort']))

        self.workLoopList = workLoopList
        self.measurementPeriod = measurementPeriod

    #----------------------------------------

    def __call__(self, args):
        logger = logging.getLogger("goalFunction")

        # the function to be optimized
        # returns the value to be minimized
        # (we return the negative rate here)

        # apply pinning
        self.workLoopList.applyPinning(args)

        # wait a moment until we start the measurements
        time.sleep(1)

        rateMean, rateStd, lastRate = self.getEventRate()

        return rateMean, rateStd, lastRate

    #----------------------------------------

    def getParameterNames(self):
        # @return a list of names of parameters to tune
        return self.workLoopList.getParameterNames()

    #----------------------------------------

    def readWorkLoopList(self):
        """this should be called after restarting the EVB
        to re-read the thread ids of the newly started
        workloops"""

        logging.info("rereading work loop data")
        self.workLoopList.fillWorkLoopData()

    #----------------------------------------

    def getEventRate(self):
        # @return (event rate mean, event rate std dev)
        # 
        # we have this as a separate function so that
        # we can get the rate before we start tuning
        rates = []

        # collect throughput statistics over the given period
        for i in range(self.measurementPeriod):
            time.sleep(1)
            
            rate = self.getEVMparam("eventRate", "unsignedInt")

            rates.append(rate)


        rateMean = np.mean(rates)
        rateStd  = np.std(rates, 
                           ddof = 1 # sample variance
                           )

        # we also return the last rate in case 
        # event building stalled
        return (rateMean, rateStd, rates[-1])
        

    #----------------------------------------

    def getEVMparam(self, paramName, paramType):
        # @return the value of a the given parameter on the EVM application
        # (e.g. current readout rate or application state)

        import messengers

        return messengers.getParam(paramName, paramType,
                                   self.evmHostPort[0], self.evmHostPort[1], 
                                   None, 
                                   "evb::EVM" ,0)
    

#----------------------------------------------------------------------
