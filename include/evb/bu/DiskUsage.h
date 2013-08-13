#ifndef _evb_bu_DiskUsage_h_
#define _evb_bu_DiskUsage_h_

#include <boost/filesystem/convenience.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

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
        const double lowWaterMark,
        const double highWaterMark
      );

      ~DiskUsage();

      /**
       * Update the information about the disk
       * Returns false if the disk cannot be accesssed
       */
      bool update();

      /**
       * Returns true if the disk usage is exceeding the high-water mark.
       * Once the high-water mark has been exceeded, the lower-water mark
       * has be be reached before it returns false again.
       */
      bool tooHigh();

      /**
       * Return the disk size in GB
       */
      double diskSizeGB();

      /**
       * Return the relative usage of the disk in percent
       */
      double relDiskUsage();


    private:

      void doStatFs();

      const boost::filesystem::path path_;
      const double lowWaterMark_;
      const double highWaterMark_;

      boost::mutex mutex_;
      int retVal_;
      struct statfs64 statfs_;
      bool tooHigh_;

      //  : path(p), highWaterMark(highWaterMark), lowWaterMark(lowWaterMark) {};
    };

    typedef boost::shared_ptr<DiskUsage> DiskUsagePtr;

  } } // namespace evb::bu

#endif // _evb_bu_DiskUsage_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
