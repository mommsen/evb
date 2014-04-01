#ifndef _evb_readoutunit_LumiBarrier_h_
#define _evb_readoutunit_LumiBarrier_h_

#include <stdexcept>
#include <stdint.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>


namespace evb {

  namespace readoutunit {

    class LumiBarrier
    {

    public:

      LumiBarrier(const uint16_t threadCount) :
      threadCount_(threadCount),
      count_(threadCount),
      lastLumiSection_(0)
      {
        if (threadCount == 0)
          throw std::invalid_argument("threadCount cannot be zero.");
      }

      void reachedLumiSection(const uint32_t lumiSection)
      {
        if ( lumiSection <= lastLumiSection_ ) return;

        boost::mutex::scoped_lock lock(mutex_);
        while ( lumiSection > lastLumiSection_ )
        {
          if (--count_ == 0)
          {
            ++lastLumiSection_;
            count_ = threadCount_;
            cond_.notify_all();
          }

          while ( lumiSection == lastLumiSection_+1 )
            cond_.wait(lock);
        }
      }

      void unblockCurrentLumiSection()
      {
        boost::mutex::scoped_lock lock(mutex_);
        ++lastLumiSection_;
        count_ = threadCount_;
        cond_.notify_all();
      }

      uint32_t getLastLumiSection() const
      {
        return lastLumiSection_;
      }

    private:

      boost::mutex mutex_;
      boost::condition_variable cond_;
      const uint16_t threadCount_;
      uint16_t count_;
      uint32_t lastLumiSection_;

    };

  } } // namespace evb::readoutunit


#endif // _evb_readoutunit_LumiBarrier_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -