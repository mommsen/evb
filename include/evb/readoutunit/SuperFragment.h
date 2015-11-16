#ifndef _evb_readoutunit_SuperFragment_h_
#define _evb_readoutunit_SuperFragment_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "evb/EvBid.h"
#include "evb/readoutunit/FedFragment.h"


namespace evb {
  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Represent a super-fragment
     */

    class SuperFragment
    {
    public:

      SuperFragment(const EvBid&, const std::string& subSystem);

      void discardFedId(const uint16_t fedId);

      bool append(const FedFragmentPtr&);

      bool hasMissingFEDs() const
      { return !missingFedIds_.empty(); }

      typedef std::vector<uint16_t> MissingFedIds;
      MissingFedIds getMissingFedIds() const
      { return missingFedIds_; }

      typedef std::vector<FedFragmentPtr> FedFragments;
      const FedFragments& getFedFragments() const
      { return fedFragments_; }

      EvBid getEvBid() const { return evbId_; }
      uint32_t getSize() const { return size_; }

    private:

      const EvBid evbId_;
      const std::string subSystem_;
      uint32_t size_;

      MissingFedIds missingFedIds_;
      FedFragments fedFragments_;

    };

    typedef boost::shared_ptr<SuperFragment> SuperFragmentPtr;

  } //namespace readoutunit
} //namespace evb

#endif // _evb_readoutunit_SuperFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
