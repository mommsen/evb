#ifndef _evb_bu_FragmentChain_h_
#define _evb_bu_FragmentChain_h_

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <stdint.h>
#include <vector>

#include "toolbox/mem/Reference.h"


namespace evb {
  namespace bu {

    /**
     * \ingroup xdaqApps
     * \brief A chain of event fragment
     */

    class FragmentChain
    {
    public:

      FragmentChain();
      FragmentChain(const uint32_t resourceCount);

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

      bool checkResourceId(const uint16_t resourceId);
      void chainFragment(toolbox::mem::Reference*);

      typedef std::vector<uint32_t> ResourceList;
      ResourceList resourceList_;
      toolbox::mem::Reference* head_;
      toolbox::mem::Reference* tail_;
      size_t size_;

      typedef std::vector<toolbox::mem::Reference*> BufRefs;
      BufRefs bufRefs_;

    }; // FragmentChain

    //    typedef FragmentChain<msg::I2O_DATA_BLOCK_MESSAGE_FRAME> FragmentChain;
    typedef boost::shared_ptr<FragmentChain> FragmentChainPtr;

  } // namespace bu
} // namespace evb

#endif // _evb_bu_FragmentChain_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
