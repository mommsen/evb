#ifndef _evb_readoutunit_ferolTCP_Tools_h_
#define _evb_readoutunit_ferolTCP_Tools_h_

#include <string>

#include "toolbox/task/WorkLoop.h"


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {
      namespace tools {
  //      extern __thread int log_level;
      
        /* The following function is used with DEBUG enabled */
        /* Returns 32-bit timestamp with us precision */
        uint32_t get_timestamp();
        uint32_t get_timestamp_rel();
        std::string get_timestamp_rel_str();

        std::string getWorkLoopThreadID(toolbox::task::WorkLoop* wl);
      }
    }
  }
}


//#define LOG(str, rest...) fprintf(stderr, "LOG[%s:%d]:%s " str "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest);

// #define LOG(str, rest...) do { 								
// 	for (int i=1; i <= evb::readoutunit::log_level; i++) fprintf(stderr,"  ");	
// 	fprintf(stderr, "LOG[%s:%d]:%s " str "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest); 
// 	} while (false)
// 
// 
// #define LOG_CALL(str, rest...) do { 							
// 	evb::readoutunit::log_level++;							
// 	for (int i=1; i <= evb::readoutunit::log_level; i++) fprintf(stderr,"  ");	
// 	fprintf(stderr, "LOG[%s:%d]:%s CALL " str "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest); 
// 	} while (false)
// 
// #define LOG_RETURN(str, rest...) do { 							
// 	for (int i=1; i <= evb::readoutunit::log_level; i++) fprintf(stderr,"  ");	
// 	fprintf(stderr, "LOG[%s:%d]:%s RETURN " str "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest); 
// 	evb::readoutunit::log_level--;							
// 	} while (false)

#define LOG(str, rest...)        fprintf(stderr, "%s: LOG[%s:%d]:%s " str "\n", evb::readoutunit::ferolTCP::tools::get_timestamp_rel_str().c_str(), __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest);

#define LOG_CALL(str, rest...)   fprintf(stderr, "%s: LOG[%s:%d]:%s CALL " str "\n", evb::readoutunit::ferolTCP::tools::get_timestamp_rel_str().c_str(), __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest);

#define LOG_RETURN(str, rest...) fprintf(stderr, "%s: LOG[%s:%d]:%s RETURN " str "\n", evb::readoutunit::ferolTCP::tools::get_timestamp_rel_str().c_str(), __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest);

#define LOG_ERROR(str, rest...)   fprintf(stderr, "%s: LOG[%s:%d]:%s ERROR " str "\n", evb::readoutunit::ferolTCP::tools::get_timestamp_rel_str().c_str(), __FILE__, __LINE__, __PRETTY_FUNCTION__, ## rest);


//#define LOG(str, rest...) fprintf(stderr, "%010u: LOG[%s:%d]: " str "\n", tools::get_timestamp(), __FILE__, __LINE__, ## rest);

//#define LOG(str, rest...) ;

#endif // _evb_readoutunit_ferolTCP_Tools_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -

