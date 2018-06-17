#!/usr/bin/env python
import os, requests, re, time
import multiprocessing as mp

from SymbolMap import SymbolMap
from messengers import sendSoapMessage, SOAPexception, webPing
from TestCase import init_worker

import numpy as np

#----------------------------------------------------------------------

def getIbvLidsInstancesFromHyperdaq(hostport):
    """parses the Hyperdaq 'view applications' page of the given application and returns
       the LID of the pt::ibv application
    :param hostport: (host, port) of the Hyperdaq application
    :return: a tuple of (LID, instance) of the IBV application (as integer) or (None, None) if not found
       """

    # this does not attempt to do any HTML parsing but relies on regexes
    # instead (and we make therefore some assupmtions about the format
    # of the HTML page)

    response = requests.get("http://%s:%d/urn:xdaq-application:service=hyperdaq/viewApplications" % (hostport[0], hostport[1]))

    # example:
    # <a href="http://dvru-c2f33-27-01.cms:25000/urn:xdaq-application:lid=201">pt::ibv::Application</a></td><td>0</td><td>201</td>
    mo = re.search('<a href=".*">pt::ibv::Application</a></td><td>(\d+)</td><td>(\d+)</td>', response.text)

    if mo:
        instance = int(mo.group(1))
        lid = int(mo.group(2))
        return (lid, instance)
    else:
        # not found
        return (None, None)

#----------------------------------------------------------------------

getIbvCountersRegex = re.compile(
        "<thead>"
        "<tr><th>Idle</th><th>Actual receive</th></tr>"
        "</thead>"
        "<tbody>"
        "<tr>"
        "<td>(\d+)</td>"  # idle counter
        "<td>(\d+)</td>"  # actual receives
        "</tr></tbody>"
    )

def getIbvCounters(hostport, lid):
    """Downloads the HTML page of the given pt::ibv application to
    extract the idle/actual receive counters from the HTML
    (these counters seem NOT to be part of the ParameterQuery response)

    :param hostport: (hostname, port) of xdaq application server
    :param lid: lid of the pt::ibv application
    :return: (timestamp, idle, succesful receives) counters
    """

    # example HTML:
    # <thead>
    # <tr><th>Idle</th><th>Actual receive</th></tr>
    # </thead>
    # <tbody>
    # <tr><td>826062430086</td><td>2326353096</td></tr></tbody></table>

    response = requests.get("http://%s:%d/urn:xdaq-application:lid=%d" % (hostport[0], hostport[1], lid))
    now = time.time()

    text = response.text.replace("\n","")

    mo = re.search(getIbvCountersRegex, text)

    if mo:
        return (now, long(mo.group(1)), long(mo.group(2)))
    else:
        return (now, None, None)

#----------------------------------------------------------------------

def getIbvCountersWrapper(args):
    """wrapper around getIbvCounters() to expand arguments so that we can use it
    with multiprocessing.map()"""
    return getIbvCounters(*args)

#----------------------------------------------------------------------

class IbvCounterMonitor:
    """
    Class to repeatedly monitor the IBV receive business of multiple
    XDAQ applications
    """

    def __init__(self, soapHostPorts):
        """
        :param soapHostPorts: list of (soapHost, soapPort) of the xdaq applications to monitor
        """

        # make a copy
        self._soapHostPorts = list(soapHostPorts)

        self._numApplications = len(self._soapHostPorts)

        self._pool = mp.Pool(20, init_worker)

        # contact all xdaq applications
        # parse the LIDs from the hyperdaq's main page
        # (this allows us to find xdaq applications independently
        # of whether they were launched from runBenchmarks.py
        # or runScans.py)

        # list of (lid, instance) of the ptv::ibv applications
        self._ibvLidsInstances = self._pool.map(getIbvLidsInstancesFromHyperdaq, hostPorts)

        # pre-pack arguments for when we run updates
        self._getterArgs = zip(self._soapHostPorts, [ item[0] for item in self._ibvLidsInstances])

        # keep track of the times of the last updates
        self._updateTimes = np.zeros(self._numApplications, dtype = 'float64')

        self._isFirstUpdate = True

        # keep track of times (in seconds) between updates
        # this can be useful for monitoring the number of total
        # read attempts
        self._deltaTimes = np.zeros(self._numApplications, dtype = 'float64')

        self._idleReceiveCounts = np.zeros(self._numApplications, dtype ='int64')

        self._successfulReceiveCounts = np.zeros(self._numApplications, dtype ='int64')

        # ratio of successful to (successful + idle) since last update
        self._successfulReceiveFraction = np.zeros(self._numApplications, dtype = 'float64')


    def update(self):
        """
        fetches the IBV counters from all applications
        """

        result = self._pool.map(getIbvCountersWrapper, self._getterArgs)

        # make copies (otherwise we modify the same arrays during the update)
        prevTimes = np.array(self._updateTimes)
        oldSuccesfulReceiveCounts = np.array(self._successfulReceiveCounts)
        oldIdleReceiveCounts = np.array(self._idleReceiveCounts)

        self._updateTimes = np.zeros(self._numApplications, dtype = 'float64')

        for i in range(self._numApplications):
            self._updateTimes[i] = result[i][0]
            self._idleReceiveCounts[i] = result[i][1]
            self._successfulReceiveCounts[i] = result[i][2]

        # update derived quantities

        if self._isFirstUpdate:
            self._isFirstUpdate = False
        else:
            # this is not the first update, we can calculate differences
            self._deltaTimes = self._updateTimes - prevTimes

            succesfullReceives = self._successfulReceiveCounts - oldSuccesfulReceiveCounts
            idleReceives = self._idleReceiveCounts - oldIdleReceiveCounts

            self._successfulReceiveFraction = succesfullReceives / (
                    succesfullReceives + idleReceives
            ).astype('float64')

#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------

if __name__ == '__main__':
    from argparse import ArgumentParser

    parser = ArgumentParser()
    try:
        symbolMapfile = os.environ["EVB_TESTER_HOME"] + '/cases/' + os.environ["EVB_SYMBOL_MAP"]
        parser.add_argument("-m", "--symbolMap", default=symbolMapfile,
                            help="symbolMap file to use, [default: %(default)s]")
    except KeyError:
        parser.add_argument("-m", "--symbolMap", required=True, help="symbolMap file to use")

    args = parser.parse_args()

    symbolMap = SymbolMap(args.symbolMap)

    # get a list of all xdaq applications defined in the SymbolMap
    # (note that not all need to be existing when running)

    hostInfos = [
        symbolMap.getHostInfo(hostType)
        for shortHostType in symbolMap.getShortHostTypes()
        for hostType in symbolMap.getHostsOfType(shortHostType)
    ]

    hostPorts = [ (hostInfo['soapHostname'], int(hostInfo['soapPort'])) for hostInfo in hostInfos ]

    # check which hosts of the SymbolMap are actually reachable
    hostPorts = [ hostInfo
                  for hostInfo in hostPorts
                  if webPing(hostInfo[0], hostInfo[1])
                  ]


    hostsForDisplay = []
    for hostPort in hostPorts:
        host = hostPort[0].lower()
        if host.endswith(".cms"):
            host = host[:-4]

        hostsForDisplay.append(host)

    ibvCounterMonitor = IbvCounterMonitor(hostPorts)

    # first update
    ibvCounterMonitor.update()

    while True:
        time.sleep(1)
        ibvCounterMonitor.update()

        print "%-30s: %-5s" % ("host", "succ. rx fraction")

        for host, successfulReceiveFraction in zip(hostsForDisplay, ibvCounterMonitor._successfulReceiveFraction * 100):
            print "%-30s: %5.1f%%" % (host, successfulReceiveFraction)

        print

