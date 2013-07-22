#include <stdint.h>
#include <stdio.h>

#include <boost/thread/thread.hpp>

#include "evb/OneToOneQueue.h"


evb::OneToOneQueue<uint32_t> queue("queue");
volatile bool running(false);
const size_t queueSize(2048);
const uint32_t testDuration(15);

void source()
{
  uint32_t counter = 0;
  while ( running )
  {
    if ( queue.enq(counter) ) ++counter;
  }
}

void sink()
{
  uint32_t counter = 0;
  uint32_t expected = 0;
  while ( running )
  {
    if ( queue.deq(counter) )
    {
      if ( counter != expected )
      {
        std::ostringstream oss;
        oss << "Dequeued " << counter << " while expecting " << expected;
        throw( oss.str() );
      }
      ++expected;
    }
  }
}

int main( int argc, const char* argv[] )
{
  queue.resize(queueSize);
  running = true;

  boost::thread sourceThread(source);
  boost::thread sinkThread(sink);

  ::sleep(testDuration);

  running = false;
  sourceThread.join();
  sinkThread.join();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
