#ifndef _evb_ru_SuperFragment_h_
#define _evb_ru_SuperFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  namespace ru {
    
    /**
     * \ingroup xdaqApps
     * \brief Represent a super fragment
     */
    
    class SuperFragment
    {
    public:

      typedef std::vector<uint16_t> FEDlist;
      
      SuperFragment();
      SuperFragment(const EvBid&, const FEDlist&);
      
      ~SuperFragment();
      
      /**
       * Append the toolbox::mem::Reference to the fragment.
       * Return false if the FED id is not expected.
       */
      bool append(uint16_t fedId, toolbox::mem::Reference*);

      /**
       * Return the head of the toolbox::mem::Reference chain
       */
      toolbox::mem::Reference* head() const
      { return head_; }

      /**
       * Return the size of the super fragment
       */
      size_t getSize() const
      { return size_; }

      /**
       * Return the event-builder id of the super fragment
       */
      EvBid getEvBid() const
      { return evbId_; }

      /**
       * Return true if there's a valid super fragment
       */
      bool isValid() const
      { return ( size_ > 0 && head_ ); }

      /**
       * Return true if the super fragment is complete
       */
      bool isComplete() const
      { return remainingFEDs_.empty(); }
      
      
    private:

      bool checkFedId(const uint16_t fedId);
      
      EvBid evbId_;
      FEDlist remainingFEDs_;
      size_t size_;
      toolbox::mem::Reference* head_;
      toolbox::mem::Reference* tail_;
      
    }; // SuperFragment
    
    typedef boost::shared_ptr<SuperFragment> SuperFragmentPtr;
    
  } } // namespace evb::ru


#endif // _evb_ru_SuperFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
