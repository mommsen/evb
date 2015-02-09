#ifndef _evb_readoutunit_ferolTCP_BufferedFragmentReader_h_
#define _evb_readoutunit_ferolTCP_BufferedFragmentReader_h_

//#include <boost/shared_ptr.hpp>

//#include <sstream>
//#include <stdint.h>
//#include <vector>

#include <boost/scoped_ptr.hpp>

#include <evb/readoutunit/ferolTCP/Exception.h>
#include <evb/readoutunit/ferolTCP/Connection.h>
#include <evb/readoutunit/ferolTCP/BufferPool.h>
#include <evb/readoutunit/ferolTCP/FragmentBuffer.h>

#include "interface/shared/ferol_header.h"

#include "evb/Constants.h"

#include <sys/socket.h>
#include <netinet/in.h>


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {

      /**
       * \ingroup xdaqApps
       * \brief A TCP Connection
       */

      class BufferedFragmentReader
      {
      public:

        BufferedFragmentReader(Connection& connection, FragmentBuffer& buffer) :
          connection_(connection),
          buffer_(buffer)
        {}

        ~BufferedFragmentReader()
        {}


        inline unsigned char* receiveNBytesBuffered(size_t readSize)
        {
          unsigned char* pointer;
          size_t toRead;
          size_t freeSpace;

          // If we have enough data in the buffer, just return them to the caller
          pointer = buffer_.getData(readSize);
          if (pointer)
          {
            return pointer;
          }

          // Here we don't have enough data to process the request, we need to receive a new data

          // How much data we need to read at minimum
          toRead = readSize - buffer_.getSizeOfData();

          // Find where we can store our data, this call may block if the buffer is full (waiting for some free space)
          pointer = buffer_.getFreeSpace((size_t)toRead, freeSpace);

          // Read something between toRead and freeSize
          int received = connection_.receiveMinMaxBytes(pointer, toRead, freeSpace);

          // Update the buffer occupancy
          buffer_.updateFreeSpace(pointer, received);

          // Return the requested amount of data
          pointer = buffer_.getData(readSize);

          return pointer;
        }


        inline int receiveFragmentWithFerolHeadersBuffered(unsigned char*& payload, uint16_t& fedID, uint32_t& eventNumber, Buffer*& buffer)
        {
          const int FEROL_HEADER = sizeof(ferolh_t);
          ferolh_t *ferolHeader;

          int to_read = 0;
          int received = 0;

          unsigned char* data = NULL; // Start of the fragment

          //TODO: Implement safety checks on FEROL header and block counts !!!!

          do {
            //
            // Receive FEROL header
            //
            ferolHeader = (ferolh_t*) receiveNBytesBuffered(FEROL_HEADER);
            received += FEROL_HEADER;

            // Store the start of the fragment
            if (!data)
              data = (unsigned char*) ferolHeader;

            if (ferolHeader->signature() != FEROL_SIGNATURE)
            {
              throw Exception(EILSEQ, "TCPFerolConnection::receiveFragmentWithFerolHeaders(): The FEROL header id mistmatch");
            }

            to_read = ferolHeader->data_length();
            assert(to_read > 8 && to_read <= (FEROL_BLOCK_SIZE - FEROL_HEADER));

            //
            // Receive FEROL block
            //
            receiveNBytesBuffered(to_read);
            received += to_read;

          } while (!ferolHeader->is_last_packet());

          if (received & 0x03)
          {
            // If received is not divisible by 8 then a FATAL ERROR happened
            LOG("The length of received data (blocks+fragment data) is %u and is  not divisible by 8, a fatal ERROR!", received);
            throw Exception(EIO, "TCPFerolConnection::receiveFragmentWithFerolHeaders(): The length of received data (blocks+fragment data) is not divisble by 8, a fatal ERROR!");
          }

          // Announce end of fragment and update the pointer to the beginning of the fragment (in case it was moved)
          data = buffer_.setEndOfFragment(received, buffer);

          //LOG("Data pointer = *%lu", ferolBuf_->getPointerOffset((unsigned char *)data));

          payload = data;
          fedID = ferolHeader->fed_id();
          eventNumber = ferolHeader->event_number();

          // Return the amount of bytes received
          return received;
        }


      private:

        Connection& connection_;
        FragmentBuffer& buffer_;

      }; // BufferedFragmentReader

      typedef boost::scoped_ptr<BufferedFragmentReader> BufferedFragmentReaderPtr;


  } } }  // namespace evb::readoutunit::ferolTCP


#endif // _evb_readoutunit_ferolTCP_BufferedFragmentReader_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
