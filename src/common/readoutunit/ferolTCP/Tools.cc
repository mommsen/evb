#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <stdint.h>
#include <unistd.h>

#include <cstdio>
#include <ctime>
#include <sstream>


#include "evb/readoutunit/ferolTCP/Tools.h"


namespace evb {
  namespace readoutunit {
    namespace ferolTCP{
      namespace tools {
  //      __thread int log_level = 0;

        volatile uint64_t index = 0;

        uint32_t tst_start = 0;
      }
    }
  }
}



/* Returns 32-bit timestamp with us precision */
uint32_t evb::readoutunit::ferolTCP::tools::get_timestamp()
{
        struct timeval tp;
        
        if (gettimeofday(&tp, NULL) < 0) {
                fprintf(stderr, "Failed to get timestamp");
                return 0;
        }
        
        return (uint32_t)(long int)((tp.tv_sec * 1000000L) + tp.tv_usec);
}


/* Returns 32-bit timestamp with us precision relative to the first timestamp returnes */
uint32_t evb::readoutunit::ferolTCP::tools::get_timestamp_rel()
{
        uint32_t tst = get_timestamp();
        
        if (tst_start == 0) 
        {
          tst_start = tst;
        }
        
        return tst-tst_start;
}


/* Returns 32-bit timestamp with us precision in string in mm:ss:usec format */
std::string evb::readoutunit::ferolTCP::tools::get_timestamp_rel_str()
{
        uint64_t i = __sync_add_and_fetch (&index, 1);
        
        uint32_t tst = get_timestamp_rel();
        uint32_t usecs = tst % 1000000;
        uint32_t secs = tst / 1000000;
        uint32_t min = secs / 60;
        
        char buf[128]; 
        ::snprintf(buf, sizeof(buf), "%u:%u.%06u[%lu]", min, secs, usecs, i);
        std::string str(buf);
        return str;
}


static pid_t gettid(void)
{
  return syscall(SYS_gettid);
}


std::string evb::readoutunit::ferolTCP::tools::getWorkLoopThreadID(toolbox::task::WorkLoop* wl)
{
  pid_t pid,tid;
  pid = getpid();
  tid = gettid();

  std::ostringstream oss;
  oss << "Workloop [" << wl->getName() << "]: pid = " << pid <<", tid = " << tid;
  
  return oss.str();
}

