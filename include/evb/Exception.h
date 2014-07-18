#ifndef _evb_Exception_h_
#define _evb_Exception_h_

#include <exception>

#include "xcept/Exception.h"

/**
 * Exception raised in case of a configuration problem
 */
XCEPT_DEFINE_EXCEPTION(evb, Configuration)

/**
 * Exception raised when receiving a corrupt event fragment
 */
XCEPT_DEFINE_EXCEPTION(evb, DataCorruption)

/**
 * Exception raised when detecting a wrong CRC checksum
 */
XCEPT_DEFINE_EXCEPTION(evb, CRCerror)

/**
 * Exception raised when failing to write data do disk
 */
XCEPT_DEFINE_EXCEPTION(evb, DiskWriting)

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
 * Exception raised when a state machine problem arises
 */
XCEPT_DEFINE_EXCEPTION(evb, FSM)

/**
 * Exception raised when an I2O problem occured
 */
XCEPT_DEFINE_EXCEPTION(evb, I2O)

/**
 * Exception raised by the EVM if the TCDS information cannot be extracted
 */
XCEPT_DEFINE_EXCEPTION(evb, TCDS)

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
 * Exception raised when a super-fragment problem occured
 */
XCEPT_DEFINE_EXCEPTION(evb, SuperFragment)

/**
 * Exception raised by issues with xdaq::WorkLoop
 */
XCEPT_DEFINE_EXCEPTION(evb, WorkLoop)

/**
 * Local exception signalling a Halt command
 */
namespace evb {
  namespace exception {
    class HaltRequested : public std::exception {};
  }
}

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
