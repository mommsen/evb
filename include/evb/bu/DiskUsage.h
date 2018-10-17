#ifndef _evb_bu_DiskUsage_h_
#define _evb_bu_DiskUsage_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/thread.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <sys/statfs.h>


namespace evb {
  namespace bu {

    /**
     * \ingroup xdaqApps
     * \brief Monitor disk usage
     */

    class DiskUsage
    {
    public:

      DiskUsage
      (
        const boost::filesystem::path& path,
        const float lowWaterMark,
        const float highWaterMark,
        const bool deleteFiles
      );

      ~DiskUsage();

      /**
       * Update the information about the disk
       * Returns false if the disk cannot be accesssed
       */
      void update();

      /**
       * Returns the change in the relative disk usage
       * btw the low and high water mark.
       */
      float overThreshold();

      /**
       * Return the disk size in GB
       */
      float diskSizeGB();

      /**
       * Return the relative usage of the disk in percent
       */
      float relDiskUsage();

      /**
       * Return the path being monitored
       */
      boost::filesystem::path path() const
      { return path_; }

    private:

      void doStatFs();
      void deleteRawFiles();

      const boost::filesystem::path path_;
      const float lowWaterMark_;
      const float highWaterMark_;
      bool deleteFiles_;

      enum State { IDLE, UPDATE, UPDATING, STOP };
      State state_;
      std::mutex mutex_;
      std::condition_variable condition_;
      boost::thread diskUsageThread_;
      boost::thread deleteThread_;
      float diskSizeGB_;
      float relDiskUsage_;
      bool valid_;
    };

    using DiskUsagePtr = std::shared_ptr<DiskUsage>;

  } } // namespace evb::bu

#endif // _evb_bu_DiskUsage_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
