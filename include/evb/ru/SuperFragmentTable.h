#ifndef _evb_ru_SuperFragmentTable_h_
#define _evb_ru_SuperFragmentTable_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <vector>
#include <stdint.h>

#include "evb/EvBid.h"
#include "evb/EvBidFactory.h"
#include "evb/InfoSpaceItems.h"
#include "evb/ru/SuperFragment.h"
#include "toolbox/mem/Reference.h"
#include "xdata/Boolean.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"


namespace evb {
  
  class RU;
  
  namespace ru {
    
    /**
     * \ingroup xdaqApps
     * \brief Keep track of complete super-fragments and BU requests
     */
    
    class SuperFragmentTable
    {
    public:
      
      SuperFragmentTable(boost::shared_ptr<RU>);
      
      /**
       * Add the data fragment from a single FED
       */
      void addFragment(toolbox::mem::Reference*);
      
      /**
       * Get the next complete super fragment.
       * If none is available, the method returns false.
       * Otherwise, the SuperFragmentPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      bool getNextAvailableSuperFragment(SuperFragmentPtr);
      
      /**
       * Get the complete super fragment with EvBid.
       * If it is not available or complete, the method returns false.
       * Otherwise, the SuperFragmentPtr holds the
       * toolbox::mem::Reference chain to the FED fragements.
       */
      bool getSuperFragmentWithEvBid(const EvBid&, SuperFragmentPtr);
      
      /**
       * Configure the SuperFragmentTable
       */
      void configure();
      
      /**
       * Append the info space parameters used for the
       * configuration to the InfoSpaceItems
       */
      void appendConfigurationItems(InfoSpaceItems&);
      
      /**
       * Append the info space items to be published in the 
       * monitoring info space to the InfoSpaceItems
       */
      void appendMonitoringItems(InfoSpaceItems&);
      
      /**
       * Update all values of the items put into the monitoring
       * info space. The caller has to make sure that the info
       * space where the items reside is locked and properly unlocked
       * after the call.
       */
      void updateMonitoringItems();
      
      /**
       * Reset the monitoring counters
       */
      void resetMonitoringCounters();
      
      /**
       * Remove all data
       */
      void clear();
      
      
    private:

      boost::shared_ptr<RU> ru_;
      EvBidFactory evbIdFactory_;
      
      typedef std::map<EvBid,SuperFragmentPtr> SuperFragmentMap;
      SuperFragmentMap superFragmentMap_;
      boost::mutex superFragmentMapMutex_;

      SuperFragment::FEDlist fedList_;
      
      uint32_t nbSuperFragmentsReadyLocal_;

      xdata::UnsignedInteger32 nbSuperFragmentsReady_;

      InfoSpaceItems superFragmentTableParams_;
      xdata::Boolean dropInputData_;
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds_;
      
    }; // SuperFragmentTable
    
  typedef boost::shared_ptr<SuperFragmentTable> SuperFragmentTablePtr;
    
  } } // namespace evb::ru


#endif // _evb_ru_SuperFragmentTable_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
