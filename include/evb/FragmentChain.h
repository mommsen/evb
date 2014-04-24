#ifndef _evb_ru_FragmentChain_h_
#define _evb_ru_FragmentChain_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "evb/I2OMessages.h"
#include "interface/shared/i2ogevb2g.h"
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

    struct Fragment
    {
      const EvBid evbId;
      toolbox::mem::Reference* bufRef;
      tcpla::MemoryCache* cache;

      Fragment(EvBid evbId, toolbox::mem::Reference* bufRef, tcpla::MemoryCache* cache)
      : evbId(evbId),bufRef(bufRef),cache(cache) {};

      ~Fragment()
      {
        // break any chain
        bufRef->setNextReference(0);
        cache->grantFrame(bufRef);
      }
    };
    typedef boost::shared_ptr<Fragment> FragmentPtr;

    typedef std::vector<uint32_t> ResourceList;

    FragmentChain();
    FragmentChain(const uint32_t resourceCount);
    FragmentChain(FragmentPtr&);
    FragmentChain(const EvBid&);
    FragmentChain(const EvBid&, toolbox::mem::Reference*);

    ~FragmentChain();

    /**
     * Append the toolbox::mem::Reference to the fragment.
     * Return false if the resource id is not expected.
     */
    bool append(uint32_t resourceId, toolbox::mem::Reference*);

    /**
     * Append the toolbox::mem::Reference to the fragment.
     */
    void append(toolbox::mem::Reference*);

    /**
     * Append the fragment. Keep track of the tcpla::MemoryCache.
     */
    void append(FragmentPtr&);

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

    uint32_t calculateSize(toolbox::mem::Reference*) const;
    bool checkResourceId(const uint32_t resourceId);
    void chainFragment(toolbox::mem::Reference*);

    EvBid evbId_;
    ResourceList resourceList_;
    size_t size_;
    toolbox::mem::Reference* head_;
    toolbox::mem::Reference* tail_;

    typedef std::vector<toolbox::mem::Reference*> BufRefs;
    BufRefs bufRefs_;

    typedef std::vector<FragmentPtr> Fragments;
    Fragments fragments_;

  }; // FragmentChain


  namespace readoutunit
  {
    typedef FragmentChain<I2O_DATA_READY_MESSAGE_FRAME> FragmentChain;
    typedef boost::shared_ptr<FragmentChain> FragmentChainPtr;
  }

  namespace bu
  {
    typedef FragmentChain<msg::I2O_DATA_BLOCK_MESSAGE_FRAME> FragmentChain;
    typedef boost::shared_ptr<FragmentChain> FragmentChainPtr;
  }

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
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId) :
evbId_(evbId),
size_(0),
head_(0),tail_(0)
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId, toolbox::mem::Reference* bufRef) :
evbId_(evbId),
size_(calculateSize(bufRef)),
head_(bufRef),tail_(bufRef)
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(FragmentPtr& fragment) :
evbId_(fragment->evbId),
size_(calculateSize(fragment->bufRef)),
head_(fragment->bufRef),tail_(fragment->bufRef)
{
  fragments_.push_back(fragment);
}


template<class T>
evb::FragmentChain<T>::~FragmentChain()
{
  for ( BufRefs::iterator it = bufRefs_.begin(), itEnd = bufRefs_.end();
        it != itEnd; ++it )
  {
    // break any chain
    (*it)->setNextReference(0);
    (*it)->release();
  }
  bufRefs_.clear();
  fragments_.clear();
}


template<class T>
uint32_t evb::FragmentChain<T>::calculateSize
(
  toolbox::mem::Reference* bufRef
) const
{
  return bufRef->getDataSize() - sizeof(T);
}


template<class T>
bool evb::FragmentChain<T>::append
(
  const uint32_t resourceId,
  toolbox::mem::Reference* bufRef
)
{
  if ( ! checkResourceId(resourceId) ) return false;

  append(bufRef);

  return true;
}


template<class T>
void evb::FragmentChain<T>::append
(
  toolbox::mem::Reference* bufRef
)
{
  chainFragment(bufRef);

  do
  {
    bufRefs_.push_back(bufRef);
    bufRef = bufRef->getNextReference();
  }
  while (bufRef);
}


template<class T>
void evb::FragmentChain<T>::append
(
  FragmentPtr& fragment
)
{
  chainFragment(fragment->bufRef);
  fragments_.push_back(fragment);
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


template<class T>
void evb::FragmentChain<T>::chainFragment
(
  toolbox::mem::Reference* bufRef
)
{
  if ( ! head_ )
    head_ = bufRef;
  else
    tail_->setNextReference(bufRef);

  do
  {
    size_ += calculateSize(bufRef);
    tail_ = bufRef;
    bufRef = bufRef->getNextReference();
  }
  while (bufRef);
}

#endif // _evb_ru_FragmentChain_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
