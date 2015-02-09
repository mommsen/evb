#ifndef _evb_readoutunit_ferolTCP_Connection_h_
#define _evb_readoutunit_ferolTCP_Connection_h_

//#include <boost/shared_ptr.hpp>

//#include <sstream>
//#include <stdint.h>
//#include <vector>

#include <evb/readoutunit/ferolTCP/Exception.h>
#include "interface/shared/ferol_header.h"

#include "evb/Constants.h"

#include <sys/socket.h>
#include <netinet/in.h>

// Used for testing 
#ifndef RECV
#  define RECV(socket, buf, len, flags)   ::recv(socket, buf, len, flags)
#endif


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {

      /**
       * \ingroup xdaqApps
       * \brief A TCP Connection
       */

      class Connection
      {
      public:

        Connection(int connection_socket, struct sockaddr_in connection_addr) :
          connection_socket_(connection_socket),
          connection_addr_(connection_addr)
        {}

        // Dummy constructor, used for testing
        Connection() :
          connection_socket_(-1)
          {}

        ~Connection()
        {
          closeSocket();
        }


        /**
         * Assign buffer for buffered readings
         */
//        void resetBuffer(TCPFerolBuf* ferolBuf)
//        {
////          ferolBuf_ = ferolBuf;
//        }
        
        /**
         * Return the peer address
         */
        struct sockaddr_in getAddr() const
        { return connection_addr_; }


        /**
         * Receive exactly size bytes from connection
         */
        inline void receiveNBytes(unsigned char *buf, int size)
        {
          register int to_read = size;
          register int rc;

          do {
            rc = RECV(connection_socket_, buf, to_read, 0);
            if (rc == to_read)
            {
                // Speedup return when we have everything
                return;
            }
            if (rc <= 0)
            {
              if (rc == 0)
              {
                // Connection was closed
                throw Exception("TCPFerolConnection::receiveNBytes: recv - Connection closed prematurely");
              }
              if (errno == EINTR)
              {
                // Read call was interrupted by a signal, it is safe to restart
                continue;
              }
              // Other error found
              throw Exception("TCPFerolConnection::receiveNBytes: recv");
            }
            buf += rc;
            to_read -= rc;
          } while (to_read > 0);
        }

        
        /**
         * Try to receive maxSize bytes from connection, return when at least minSize bytes is received
         * Return the number of bytes received (at least minSize)
         */
        inline int receiveMinMaxBytes(unsigned char *buf, int minSize, int maxSize)
        {
          register int rc;
          register int total = 0;

          // HACK:
          //maxSize = 2100;

  //        LOG_CALL("buf = %p, minSize = %u, maxSize = %u", buf, minSize, maxSize);

          assert(minSize <= maxSize);

          do {
            rc = RECV(connection_socket_, buf, maxSize, 0);
            if (rc <= 0)
            {
              if (rc == 0)
              {
                // Connection was closed
                throw Exception("TCPFerolConnection::receiveNBytes: recv - Connection closed prematurely");
              }
              if (errno == EINTR)
              {
                // Read call was interrupted by a signal, it is safe to restart
                continue;
              }
              // Other error found
              throw Exception("TCPFerolConnection::receiveNBytes: recv");
            }
            total += rc;
            buf += rc;
            maxSize -= rc;
            minSize -= rc;
          } while (minSize > 0);

  //        LOG_RETURN("%u", total);
          return total;
        }


        /**
         * Recevies one full fragment from connection into the buffer and fills it up to the bufsize
         * Return the number of bytes received from the connection (includes headers + blocks)
         */
        inline int receiveFragmentWithFerolHeaders(unsigned char *buf, int bufsize, uint16_t& fedID, uint32_t& eventNumber)
        {
          const int FEROL_HEADER = sizeof(ferolh_t);
          int to_read = 0;
          ferolh_t *ferolHeader;
          int bufsize_backup = bufsize;

          //TODO: Implement safety checks on FEROL header and block counts !!!!

          do {
            ferolHeader = (ferolh_t*) buf;

            //
            // Receive FEROL header
            //
            if (bufsize < FEROL_HEADER)
            {
              throw Exception(EMSGSIZE, "TCPFerolConnection::receiveFragmentWithFerolHeaders: The fragment will not fit into the buffer, increase the buffer size");
            }
            receiveNBytes(buf, FEROL_HEADER);
            buf += FEROL_HEADER;
            bufsize -= FEROL_HEADER;

            if (ferolHeader->signature() != FEROL_SIGNATURE)
            {
              throw Exception(EILSEQ, "TCPFerolConnection::receiveFragmentWithFerolHeaders: The FEROL header id mismatch");
            }

            to_read = ferolHeader->data_length();
            assert(to_read > 8 && to_read <= (FEROL_BLOCK_SIZE - FEROL_HEADER));

            //
            // Receive FEROL block
            //
            if (bufsize < to_read)
            {
              throw Exception(EMSGSIZE, "TCPFerolConnection::receiveFragmentWithFerolHeaders: The fragment will not fit into the buffer, increse the buffer size");
            }
            receiveNBytes(buf, to_read);
            buf += to_read;
            bufsize -= to_read;

          } while (!ferolHeader->is_last_packet());

          fedID = ferolHeader->fed_id();
          eventNumber = ferolHeader->event_number();

          // Return the amount of bytes received
          return (bufsize_backup - bufsize);
        }

      private:

        void closeSocket()
        {
          if (connection_socket_ >= 0) {
            ::close(connection_socket_);
            connection_socket_ = -1;
          }
        }


        int connection_socket_;
        struct sockaddr_in connection_addr_;

      }; // Connection

      typedef boost::shared_ptr<Connection> ConnectionPtr;


  } } }  // namespace evb::readoutunit::ferolTCP


#endif // _evb_readoutunit_ferolTCP_Connection_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
