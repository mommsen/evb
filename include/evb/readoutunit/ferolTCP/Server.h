#ifndef _evb_readoutunit_ferolTCP_Server_h_
#define _evb_readoutunit_ferolTCP_Server_h_

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/I2OMessages.h"
#include "interface/shared/ferol_header.h"

#include <cerrno>

#include <sys/socket.h>
#include <netinet/in.h>

#include <evb/readoutunit/ferolTCP/Connection.h>
#include <evb/readoutunit/ferolTCP/Exception.h>


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {

      /**
       * \ingroup xdaqApps
       * \brief A TCP receiver which receives for the connections from FEROL
       */

      class Server
      {
      public:

        Server() :
          server_socket_(-1)
        {}

        ~Server()
        {
          closeSocket();
        }

        /**
         * Start the server running on the specified port number
         */
        void startServer(int server_port)
        {
          int sock;
          struct sockaddr_in server_addr;
          int var_true = 1;

          closeSocket();

          if ((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
          {
            throw Exception("Server::startServer: socket");
          }

          // set SO_REUSEADDR on a socket to bypass TIME_WAIT state
          if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &var_true, sizeof(int)) == -1)
          {
            int e = errno;
            closeSocket();
            throw Exception(e, "Server::startServer: setsockopt");
          }

          server_addr.sin_family = AF_INET;
          server_addr.sin_port = htons(server_port);
          server_addr.sin_addr.s_addr = INADDR_ANY;

          // TODO: Put here some constant
          ::bzero(&(server_addr.sin_zero), 8);

          if (::bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
          {
            int e = errno;
            closeSocket();
            throw Exception(e, "Server::startServer: Unable to bind");
          }

          if (listen(sock, 1) == -1)
          {
            int e = errno;
            closeSocket();
            throw Exception(e, "Server::startServer: listen");
          }

          server_socket_ = sock;
        }


        /**
         * Start the server running on the specified port number
         */
        Connection *waitForConnection() const
        {
          int client_socket;
          struct sockaddr_in client_addr;
          unsigned int sin_size;
        
          sin_size = sizeof(struct sockaddr_in);
          client_socket = ::accept(server_socket_, (struct sockaddr *)&client_addr, &sin_size);
          return new Connection(client_socket, client_addr);
        }


      private:

        void closeSocket()
        {
          if (server_socket_ >= 0)
          {
            ::close(server_socket_);
            server_socket_ = -1;
          }
        }

        int server_socket_;
      
      }; // Server

  } } }  // namespace evb::readoutunit::ferolTCP


#endif // _evb_readoutunit_ferolTCP_Server_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -

