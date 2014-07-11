#ifndef _evb_FragmentChain_h_
#define _evb_FragmentChain_h_

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/FedFragment.h"
#include "evb/I2OMessages.h"
#include "interface/shared/i2ogevb2g.h"
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

    FragmentChain();
    FragmentChain(const uint32_t resourceCount);
    FragmentChain(const EvBid&, FedFragmentPtr&);
    FragmentChain(const EvBid&);
    FragmentChain(const EvBid&, toolbox::mem::Reference*);

    ~FragmentChain();

    /**
     * Append the toolbox::mem::Reference to the fragment.
     * Return false if the resource id is not expected.
     */
    bool append(uint16_t resourceId, toolbox::mem::Reference*);

    /**
     * Append the toolbox::mem::Reference to the fragment.
     */
    void append(toolbox::mem::Reference*);

    /**
     * Append the fragment. Keep track of the tcpla::MemoryCache.
     */
    void append(const EvBid&, FedFragmentPtr&);

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
    bool checkResourceId(const uint16_t resourceId);
    void chainFragment(toolbox::mem::Reference*);

    EvBid evbId_;
    typedef std::vector<uint32_t> ResourceList;
    ResourceList resourceList_;
    toolbox::mem::Reference* head_;
    toolbox::mem::Reference* tail_;
    size_t size_;

    typedef std::vector<toolbox::mem::Reference*> BufRefs;
    BufRefs bufRefs_;

    typedef std::vector<FedFragmentPtr> FedFragments;
    FedFragments fedFragments_;

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
  head_(0),tail_(0),
  size_(0)
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(const uint32_t resourceCount) :
  head_(0),tail_(0),
  size_(0)
{
  for (uint32_t i = 1; i <= resourceCount; ++i)
  {
    resourceList_.push_back(i);
  }
}


template<class T>
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId) :
evbId_(evbId),
head_(0),tail_(0),
size_(0)
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId, toolbox::mem::Reference* bufRef) :
  evbId_(evbId),
  head_(bufRef),tail_(bufRef),
  size_(calculateSize(bufRef))
{}


template<class T>
evb::FragmentChain<T>::FragmentChain(const EvBid& evbId, FedFragmentPtr& fedFragment) :
evbId_(evbId),
head_(fedFragment->getBufRef()),
tail_(head_),
size_(calculateSize(head_))
{
  fedFragments_.push_back(fedFragment);
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
  fedFragments_.clear();
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
  const uint16_t resourceId,
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
  const EvBid& evbId,
  FedFragmentPtr& fedFragment
)
{
  if ( evbId != evbId_ )
  {
    std::ostringstream oss;
    oss << "Mismatch detected: expected evb id "
      << evbId_ << ", but found evb id "
      << evbId << " in data block from FED "
      << fedFragment->getFedId();
    XCEPT_RAISE(exception::MismatchDetected, oss.str());
  }

  chainFragment(fedFragment->getBufRef());
  fedFragments_.push_back(fedFragment);
}


template<class T>
bool evb::FragmentChain<T>::checkResourceId(const uint16_t resourceId)
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

#endif // _evb_FragmentChain_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
