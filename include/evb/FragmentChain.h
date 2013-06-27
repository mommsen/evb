#ifndef _evb_ru_FragmentChain_h_
#define _evb_ru_FragmentChain_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "pt/utcp/frl/MemoryCache.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  
  /**
   * \ingroup xdaqApps
   * \brief A chain of event fragment
   */
  
  class FragmentChain
  {
  public:
    
    typedef std::vector<uint32_t> ResourceList;
    
    FragmentChain();
    FragmentChain(const uint32_t resourceCount);
    FragmentChain(const EvBid&, const ResourceList&);
    
    ~FragmentChain();
    
    /**
     * Append the toolbox::mem::Reference to the fragment.
     * Return false if the resource id is not expected.
     */
    bool append(uint32_t resourceId, toolbox::mem::Reference*);
    bool append(uint32_t resourceId, toolbox::mem::Reference*, pt::utcp::frl::MemoryCache*);
    
    /**
     * Return the head of the toolbox::mem::Reference chain
     */
    toolbox::mem::Reference* head() const
    { return head_; }
    
    /**
     * Return a duplicate of the toolbox::mem::Reference chain
     */
    toolbox::mem::Reference* duplicate() const
    { return head_->duplicate(); }
    
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
    { return resourceList_.empty(); }
    
    
  private:
    
    bool checkResourceId(const uint32_t resourceId);
    
    EvBid evbId_;
    ResourceList resourceList_;
    size_t size_;
    toolbox::mem::Reference* head_;
    toolbox::mem::Reference* tail_;

    typedef std::map<toolbox::mem::Reference*,pt::utcp::frl::MemoryCache*> Caches;
    Caches caches_;
    
  }; // FragmentChain
  
  typedef boost::shared_ptr<FragmentChain> FragmentChainPtr;
  typedef std::vector<FragmentChainPtr> SuperFragments;
  
  } // namespace evb


#endif // _evb_ru_FragmentChain_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
