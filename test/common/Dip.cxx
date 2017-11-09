#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <boost/scoped_ptr.hpp>

#include "dip/Dip.h"
#include "dip/DipData.h"
#include "dip/DipFactory.h"
#include "dip/DipQuality.h"
#include "dip/DipSubscription.h"
#include "dip/DipSubscriptionListener.h"


class Listener : public DipSubscriptionListener
{

public:

  Listener()
  {
    //setDipNS();
    dipFactory_ = Dip::create("dipTest_evb-unittests");
    dipFactory_->setDNSNode("cmsdimns1.cern.ch,cmsdimns2.cern.ch");
    subscribeToDip();
  }

  ~Listener()
  {
    dipFactory_->destroyDipSubscription(dipSubcription_);
  }

  void handleMessage(DipSubscription* subscription, DipData& dipData)
  {
    const std::string topicName = subscription->getTopicName();
    if ( topicName == "dip/CMS/BRIL/Luminosity" )
    {
      if ( dipData.extractDataQuality() == DIP_QUALITY_GOOD )
      {
        std::cout << dipData.extractInt("LumiSection")
          << "\t" << dipData.extractInt("LumiNibble")
          << "\t" << dipData.extractFloat("InstLumi")
          << "\t" << dipData.extractFloat("AvgPileUp")
          << std::endl;
      }
    }
  }

  void disconnected(DipSubscription*, char*) {};

  void connected(DipSubscription* subscription)
  {
    std::cout << "Connected to " << subscription->getTopicName() << std::endl;
  };

  void handleException(DipSubscription*, DipException&) {};


private:

  void setDipNS()
  {
    char line [1024];
    FILE *fptr = fopen("/nfshome0/mommsen/tmp/server.conf","r");
    if ( fptr != NULL )
    {
      if ( fgets(line,sizeof(line)-1,fptr) )
      {
        int ii = strlen(line);
        printf("DIPNS set to %s", line);
        if ( ii > 0 )
        {
          line[ii-1] = '\0';
          if ( line[ii-2] < 32 ) { line[ii-2] = '\0';}
          setenv("DIPNS",line,1);
          setenv("DIM_DNS_NODE",line,1);
          setenv("DIM_DNS_PORT","2506",1);
        }
      }
      fclose(fptr);
    }
  }

  void subscribeToDip()
  {
    dipSubcription_ = dipFactory_->createDipSubscription("dip/CMS/BRIL/Luminosity",this);
  }

  DipFactory* dipFactory_;
  DipSubscription* dipSubcription_;

};


int main( int argc, const char* argv[] )
{
  Listener listener;
  sleep(300);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
