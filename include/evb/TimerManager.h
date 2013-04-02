#ifndef _evb_TimerManager_h_
#define _evb_TimerManager_h_

#include <string>
#include <stdlib.h>
#include <sys/time.h>


namespace evb {

class TimerManager
{
private:

    struct TimerTime
    {
        unsigned int  hi, lo;

        TimerTime() :
        hi(0),
        lo(0)
        {
        }
    };

    struct Timer
    {
        bool isFired;
        TimerTime deltaTime;
        TimerTime startTime;

        Timer() :
        isFired(true)
        {
        }
    };

    int            timerListMax;
    int            nextFreeTimer;
    struct timeval theTimeOfDay;
    TimerTime      theTime;

    Timer *timerList;


public:

    TimerManager();

    ~TimerManager();

    int getTimer();

    void initTimer(int handle, int dt_msec);

    void restartTimer(int handle);

    bool isFired(int handle);


private:

    void expandTimerList();

    int get_cpu_khz();

}; // class TimerManage


/**
 * Returns the delta of the two specified times in seconds.
 */
double calcDeltaTime
(
    const struct timeval *start,
    const struct timeval *end
);

/**
 * Returns the current time in UTC (thread safe)
 */
std::string getCurrentTimeUTC();

} // namespace evb

#endif
