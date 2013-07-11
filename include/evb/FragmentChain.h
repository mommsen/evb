#ifndef _evb_ru_FragmentChain_h_
#define _evb_ru_FragmentChain_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "tcpla/MemoryCache.h"
#include "toolbox/mem/Reference.h"


namespace evb {
  
  /**
   * \ingroup xdaqApps
   * \brief A chain of event fragment
   */
  
  template<class T>
  class FragmentChain
  {
  public:
    
    typedef std::vector<uint32_t> ResourceList;
    
    FragmentChain();
    FragmentChain(const uint32_t resourceCount);
    FragmentChain(const EvBid&, const ResourceList&);
    FragmentChain(const EvBid&, toolbox::mem::Reference*);
    
    ~FragmentChain();
    
    /**
     * Append the toolbox::mem::Reference to the fragment.
     * Return false if the resource id is not expected.
     */
    bool append(uint32_t resourceId, toolbox::mem::Reference*);
    bool append(uint32_t resourceId, toolbox::mem::Reference*, tcpla::MemoryCache*);
    
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
    { return resourceList_.empty(); }
    
    
  private:
    
    bool checkResourceId(const uint32_t resourceId);
    
    EvBid evbId_;
    ResourceList resourceList_;
    size_t size_;
    toolbox::mem::Reference* head_;
    toolbox::mem::Reference* tail_;

    typedef std::map<toolbox::mem::Reference*,tcpla::MemoryCache*> Caches;
    Caches caches_;
    
  }; // FragmentChain
  
} // namespace evb


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class T>
evb::FragmentChain<T>::FragmentChain() :
size_(0),
head_(0),tail_(0)
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(const uint32_t resourceCount) :
size_(0),
head_(0),tail_(0)
{
  for (uint32_t i = 1; i <= resourceCount; ++i)
  {
    resourceList_.push_back(i);
  }
}


template<class T>
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId, const ResourceList& resourceList) :
evbId_(evbId),
resourceList_(resourceList),
size_(0),
head_(0),tail_(0)
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId, toolbox::mem::Reference* bufRef) :
evbId_(evbId),
size_(bufRef->getDataSize() - sizeof(T)),
head_(bufRef),tail_(bufRef)
{}


template<class T>
evb::FragmentChain<T>::~FragmentChain()
{
  if (head_) head_->release();
  
  for (Caches::iterator it = caches_.begin(), itEnd = caches_.end();
       it != itEnd; ++it)
  {
    it->second->grantFrame(it->first);
  }
  caches_.clear();
}


template<class T>
bool evb::FragmentChain<T>::append
(
  const uint32_t resourceId,
  toolbox::mem::Reference* bufRef
)
{
  if ( ! checkResourceId(resourceId) ) return false;

  if (!head_)
  {
    head_ = bufRef;
    tail_ = bufRef;
  }
  else
  {
    tail_->setNextReference(bufRef);
    tail_ = bufRef;
  }
  
  size_ += bufRef->getDataSize() - sizeof(T);
  
  // while ( tail_->getNextReference() )
  // {
  //   tail_ = tail_->getNextReference();
  //   size_ += tail_->getDataSize();
  // }
  
  return true;
}
  

template<class T>
bool evb::FragmentChain<T>::append
(
  const uint32_t resourceId,
  toolbox::mem::Reference* bufRef,
  tcpla::MemoryCache* cache
)
{
  if ( append(resourceId, bufRef->duplicate()) )
  {
    caches_.insert(Caches::value_type(bufRef,cache));
    return true;
  }
  
  return false;
}


template<class T>
bool evb::FragmentChain<T>::checkResourceId(const uint32_t resourceId)
{
  ResourceList::iterator pos =
    std::find(resourceList_.begin(),resourceList_.end(),resourceId);

  if ( pos == resourceList_.end() ) return false;

  resourceList_.erase(pos);

  return true;
}


#endif // _evb_ru_FragmentChain_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
