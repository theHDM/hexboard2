#pragma once
#include "hardware/timer.h"

using timeStamp = unsigned long long int;

timeStamp getTheCurrentTime() {
  timeStamp temp = timer_hw->timerawh;
  return (temp << 32) | timer_hw->timerawl;
}

class softTimer {
  private:
    timeStamp startTime;
    timeStamp delay_uS;
    bool running;
    bool finishNow;
  public:
    softTimer() {
      startTime = 0;
      delay_uS = 0;
      running = false;
      finishNow = false;
    };
    void start(timeStamp _delay_uS, timeStamp _defer_uS) {
      startTime = getTheCurrentTime() + _defer_uS;
      delay_uS = _delay_uS;
      running = true;
      finishNow = false;
    };
    void stop() {
      running = false;
      finishNow = false;
    }
    void repeat() {
      startTime = startTime + delay_uS;
      running = true;
      finishNow = false;  
    }
    void restart() {
      start(delay_uS, 0);
    }
    void finish() {
      finishNow = true;
    }
    bool justFinished() {
      if (running && (finishNow || (getElapsed() >= delay_uS))) {
        stop();
        return true;
      } // else {
      return false;  
    }
    bool isRunning() {
      return running;
    }
    bool ifDone_thenRepeat() {
      if (justFinished()) {
          repeat();
          return true;
      }
      return false;
    }    
    timeStamp getStartTime() {
      return startTime;  
    }
    timeStamp getElapsed() {
      timeStamp temp = getTheCurrentTime();
      return (temp < startTime ? 0 : temp - startTime);
    }
    timeStamp getRemaining() {
      if (running) {
        timeStamp temp = getElapsed();
        if (finishNow || (temp >= delay_uS)) {
          return 0;
        } else {
          return (delay_uS - temp);
        }
      } else {
        return 0;
      }  
    }
    timeStamp getDelay()  {
      return delay_uS;
    }
};
