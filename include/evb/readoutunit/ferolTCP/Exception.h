#ifndef _evb_readoutunit_ferolTCP_Exception_h_
#define _evb_readoutunit_ferolTCP_Exception_h_

#include <cstring>
#include <cerrno>
#include <exception>


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {
  
      class Exception : public std::exception
      {
      public:

        Exception(const std::string& what_arg) : what_message_(what_arg + ": " + tools_strerror(errno)), error_code_(errno) {}
        Exception(const char *what_arg) : what_message_(std::string(what_arg) + ": " + tools_strerror(errno)), error_code_(errno) {}

        Exception(int error_code, const std::string& what_arg) : what_message_(what_arg + ": " + tools_strerror(error_code)), error_code_(error_code) {}
        Exception(int error_code, const char *what_arg) : what_message_(std::string(what_arg) + ": " + tools_strerror(error_code)), error_code_(error_code) {}

        virtual ~Exception() throw () {}
        virtual const char* what() const throw() { return what_message_.c_str(); }

        /**
         * Returns a C++ std::string describing ERRNO
         */
        static const std::string tools_strerror(int error_code)
        {
          char local_buf[128];  // TODO: Put here some well defined constant
          char *buf = local_buf;
          buf = ::strerror_r(error_code, buf, sizeof(local_buf));
          std::string str(buf);
          return str;
        }

      protected:
        std::string what_message_;
        int error_code_;

      }; // Exception


  } } } // namespace evb::readoutunit::ferolTCP


#endif // _evb_readoutunit_ferolTCP_Exception_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -

