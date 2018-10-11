#ifndef _evb_bu_FragmentChain_h_
#define _evb_bu_FragmentChain_h_

#include <memory>
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

      FragmentChain(uint32_t blockCount);

      ~FragmentChain();

      /**
      * Append the toolbox::mem::Reference to the fragment.
      */
      bool append(toolbox::mem::Reference*);

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

    private:

      void chainFragment(toolbox::mem::Reference*);

      uint32_t blockCount_;
      toolbox::mem::Reference* head_;
      toolbox::mem::Reference* tail_;
      size_t size_;

    }; // FragmentChain

    using FragmentChainPtr = std::shared_ptr<FragmentChain>;

  } // namespace bu
} // namespace evb

#endif // _evb_bu_FragmentChain_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
