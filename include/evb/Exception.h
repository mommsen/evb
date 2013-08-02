#ifndef _evb_Exception_h_
#define _evb_Exception_h_

#include "xcept/Exception.h"

/**
 * Exception raised in case of a configuration problem
 */
XCEPT_DEFINE_EXCEPTION(evb, Configuration)

/**
 * Exception raised by FragmentSets on a DOM error.
 */
XCEPT_DEFINE_EXCEPTION(evb, DOM)

/**
 * Exception raised when failing to provide dummy data
 */
XCEPT_DEFINE_EXCEPTION(evb, DummyData)

/**
 * Exception raised when encountering an out-of-order event
 */
XCEPT_DEFINE_EXCEPTION(evb, EventOrder)

/**
 * Exception raised by queue templates
 */
XCEPT_DEFINE_EXCEPTION(evb, FIFO)

/**
 * Exception raised by FragmentSets if a fragment is not found.
 */
XCEPT_DEFINE_EXCEPTION(evb, FragmentNotFound)

/**
 * Exception raised when a state machine problem arises
 */
XCEPT_DEFINE_EXCEPTION(evb, FSM)

/**
 * Exception raised when an I2O problem occured
 */
XCEPT_DEFINE_EXCEPTION(evb, I2O)

/**
 * Exception raised when encountering an info-space problem
 */
XCEPT_DEFINE_EXCEPTION(evb, InfoSpace)

/**
 * Exception raised by the EVM when encountering a L1 scaler problem
 */
XCEPT_DEFINE_EXCEPTION(evb, L1Scalers)

/**
 * Exception raised by the EVM when encountering a L1 trigger problem
 */
XCEPT_DEFINE_EXCEPTION(evb, L1Trigger)

/**
 * Exception raised when a super-fragment mismatch occured
 */
XCEPT_DEFINE_EXCEPTION(evb, MismatchDetected)

/**
 * Exception raised when encountering a problem providing monitoring information
 */
XCEPT_DEFINE_EXCEPTION(evb, Monitoring)

/**
 * Exception raised when running out of memory
 */
XCEPT_DEFINE_EXCEPTION(evb, OutOfMemory)

/**
 * Exception raised when on response to a SOAP message
 */
XCEPT_DEFINE_EXCEPTION(evb, SOAP)

/**
 * Exception raised when receiving a corrupt FEROL fragment
 */
XCEPT_DEFINE_EXCEPTION(evb, FEROL)

/**
 * Exception raised when a super-fragment problem occured
 */
XCEPT_DEFINE_EXCEPTION(evb, SuperFragment)

/**
 * Exception raised when a super-fragment timed-out
 */
XCEPT_DEFINE_EXCEPTION(evb, TimedOut)

/**
 * Exception raised by issues with xdaq::WorkLoop
 */
XCEPT_DEFINE_EXCEPTION(evb, WorkLoop)

/**
 * Exception raised when failing to write data do disk
 */
XCEPT_DEFINE_EXCEPTION(evb, DiskWriting)

/**
 * Exception raised when failing to create XGI documents
 */
XCEPT_DEFINE_EXCEPTION(evb, XGI)


#endif
