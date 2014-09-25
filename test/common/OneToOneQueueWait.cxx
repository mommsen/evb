#include <stdint.h>
#include <stdio.h>
#include <string>

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>

#include "cgicc/HTMLClasses.h"
#include "evb/OneToOneQueue.h"

class EvBApplication
{
public:
  void registerQueueCallback(const std::string name, boost::function<cgicc::div()>) {};
  std::string getURN() { return "urn:dummy:foo"; }
} evbApplication;


evb::OneToOneQueue<uint32_t> queue(&evbApplication,"queue");
volatile bool generating(false);
volatile bool consuming(false);
const size_t queueSize(2048);
const uint32_t testDuration(15);

void source()
{
  uint32_t counter = 0;
  while ( generating )
  {
    queue.enqWait(counter);
    ++counter;
  }
  std::cout << "Enqueued " << counter << " elements" << std::endl;
}

void sink()
{
  uint32_t counter = 0;
  uint32_t expected = 0;

  while ( consuming || !queue.empty() )
  {
    queue.deqWait(counter);

    if ( counter != expected )
    {
      std::ostringstream oss;
      oss << "Dequeued " << counter << " while expecting " << expected;
      throw( oss.str() );
    }
    ++expected;
  }
  std::cout << "Dequeued " << expected << " elements" << std::endl;
}

int main( int argc, const char* argv[] )
{
  queue.resize(queueSize);
  generating = true;
  consuming = true;

  boost::thread sourceThread(source);
  boost::thread sinkThread(sink);

  ::sleep(testDuration);

  std::cout << "Stopping source thread..." << std::endl;
  generating = false;
  sourceThread.join();

  std::cout << "Stopping sink thread..." << std::endl;
  consuming = false;
  sinkThread.join();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
