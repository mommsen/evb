#include "evb/TimerManager.h"
#include "evb/Exception.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>


evb::TimerManager::TimerManager():
timerListMax(5),
nextFreeTimer(0)
{
  timerList = new Timer[timerListMax]();
}


evb::TimerManager::~TimerManager()
{
  delete[] timerList;
}


int evb::TimerManager::getTimer()
{
  if( nextFreeTimer == timerListMax ) expandTimerList();
  return nextFreeTimer++;
}


void evb::TimerManager::initTimer(int handle, int dt_msec)
{
  if( handle >= nextFreeTimer || handle < 0 )
  {
    XCEPT_RAISE(exception::Configuration,
      "Requested to initilize non existing timer");
  }
  
  timerList[handle].deltaTime.hi = (unsigned int )(dt_msec)/1000;
  timerList[handle].deltaTime.lo = (unsigned int )(dt_msec)%1000;
  timerList[handle].isFired = false;
  //    cout << "Delta for timer " << handle << " Is " <<  
  //   (int)(timerList[handle].deltaTime.hi) << " " << 
  //(int)(timerList[handle].deltaTime.lo) << "\n";  
  
  return;
}


void evb::TimerManager::restartTimer(int handle)
{
  timerList[handle].isFired = false;
  gettimeofday(&theTimeOfDay, NULL);
  
  timerList[handle].startTime.hi =
    (unsigned int )(theTimeOfDay.tv_sec);  
  timerList[handle].startTime.lo =
    (unsigned int )(theTimeOfDay.tv_usec);      
  // cout << "start time " << timerList[handle].startTime.hi << ". "
  //      << timerList[handle].startTime.lo << "\n";
}


bool evb::TimerManager::isFired(int handle)
{
  gettimeofday(&theTimeOfDay, NULL);
  theTime.hi = (unsigned int )(theTimeOfDay.tv_sec);
  theTime.lo = (unsigned int )(theTimeOfDay.tv_usec);
  
  //cout << "time now is " << theTime.hi << ". " <<theTime.lo << "\n";
  
  unsigned int  carry = 0;
  if(theTime.lo <  timerList[handle].startTime.lo) carry = 1;
  timerList[handle].isFired =
    timerList[handle].isFired
    || 
    theTime.hi - timerList[handle].startTime.hi - carry
    > timerList[handle].deltaTime.hi
    ||
    (theTime.hi - timerList[handle].startTime.hi - carry
      == timerList[handle].deltaTime.hi 
      && theTime.lo- timerList[handle].startTime.lo > 
      timerList[handle].deltaTime.lo);
  
  return timerList[handle].isFired;
}


void evb::TimerManager::expandTimerList()
{
  int oldMax = timerListMax;
  // cout << "initial size " << timerListMax << "\n";
  timerListMax +=5;
  Timer *newTimerList;
  newTimerList = new Timer[timerListMax]();
  
  for(int i=0; i<oldMax;i++) {
    newTimerList[i].isFired = timerList[i].isFired;
    newTimerList[i].deltaTime.hi = timerList[i].deltaTime.hi;
    newTimerList[i].deltaTime.lo = timerList[i].deltaTime.lo;
    newTimerList[i].startTime.hi = timerList[i].startTime.hi;
    newTimerList[i].startTime.lo = timerList[i].startTime.lo;
  }
  
  delete[] timerList;
  timerList = newTimerList;
}


int evb::TimerManager::get_cpu_khz()
{
  FILE *in;
  char str[200];
  double cpu_mhz;
  
  if ((in = fopen("/proc/cpuinfo", "r")) == NULL) {
    perror("fopen(/proc/cpuinfo)");
    return 0;
  }
  
  while (fgets(str, sizeof(str), in) != NULL)
    if (fscanf(in, "cpu MHz : %lf\n", &cpu_mhz) == 1)
      break;
  
  fclose(in);
  return (int)(cpu_mhz * 1000); //retun cpu in khz
}


double evb::calcDeltaTime
(
  const struct timeval *start,
  const struct timeval *end
)
{
  unsigned int  sec;
  unsigned int  usec;
  
  
  sec = abs(end->tv_sec - start->tv_sec);
  usec = abs(end->tv_usec - start->tv_usec);
  
  return ((double)sec) + ((double)usec) / 1000000.0;
}


std::string evb::getCurrentTimeUTC()
{
  time_t now;
  struct tm tm;
  char buf[30];
  
  if (time(&now) != ((time_t)-1))
  {
    gmtime_r(&now,&tm);
    asctime_r(&tm,buf);
  }
  return buf;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
