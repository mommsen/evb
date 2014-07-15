#ifndef _evb_readoutunit_ScalerHandler_h_
#define _evb_readoutunit_ScalerHandler_h_

#include <stdint.h>
#include <vector>

#include "evb/CRCCalculator.h"
#include "evb/EvBid.h"
#include "evb/Exception.h"
#include "evb/FedFragment.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WorkLoop.h"


namespace evb {

  namespace readoutunit {

    /**
     * \ingroup xdaqApps
     * \brief Retrieve scaler information for softRU
     */

    class ScalerHandler : public toolbox::lang::Class
    {

    public:

      ScalerHandler(const std::string& identifier);

      void configure(const uint16_t fedId, const uint32_t dummyScalFedSize);

      void startProcessing(const uint32_t runNumber);

      void stopProcessing();

      uint32_t fillFragment(const EvBid&, FedFragmentPtr&);


    private:

      void getFragmentPool(const std::string& identifier);
      void startRequestWorkloop(const std::string& identifier);
      uint32_t getFerolFragment(const EvBid&, FedFragmentPtr&);
      bool scalerRequest(toolbox::task::WorkLoop*);
      void requestLastestData();

      uint16_t fedId_;
      uint32_t dummyScalFedSize_;
      uint32_t runNumber_;
      uint32_t currentLumiSection_;
      bool doProcessing_;
      bool dataIsValid_;

      toolbox::mem::Pool* fragmentPool_;
      toolbox::task::WorkLoop* scalerRequestWorkLoop_;
      toolbox::task::ActionSignature* scalerRequestAction_;

      CRCCalculator crcCalculator_;

      typedef std::vector<char> Buffer;
      Buffer dataForNextLumiSection_;
      Buffer dataForCurrentLumiSection_;

    };

  } } // namespace evb::readoutunit

#endif // _evb_readoutunit_ScalerHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
