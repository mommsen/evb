#ifndef _evb_bu_LumiBarrier_h_
#define _evb_bu_LumiBarrier_h_

#include <map>
#include <stdint.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>


namespace evb {

  namespace bu {

    class LumiBarrier
    {

    public:

      LumiBarrier(const uint16_t threadCount);

      /**
       * Block execution of calling thread until all threads have
       * reached this or a later lumiSection
       */
      void reachedLumiSection(const uint32_t lumiSection);

      /**
       * Unblock all threads waiting for the current lumiSection
       */
      void unblockCurrentLumiSection();

      /**
       * Unblock all threads waiting for any lumiSection up to the specified one
       */
      void unblockUpToLumiSection(const uint32_t lumiSection);

      /**
       * Return the current lumi section
       */
      uint32_t getLastLumiSection() const
      { return lastLumiSection_; }

    private:

      void waitForLumiSection(const uint32_t lumiSection);

      boost::mutex mutex_;
      boost::condition_variable cond_;
      const uint16_t threadCount_;
      typedef std::map<uint32_t,uint16_t> CountPerLS;
      CountPerLS countPerLS_;
      uint32_t lastLumiSection_;

    };

  } } // namespace evb::bu


#endif // _evb_bu_LumiBarrier_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
