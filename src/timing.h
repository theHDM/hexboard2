#pragma once
#include "utils.h"
#include "hardware/timer.h"

time_uS getTheCurrentTime() {
  time_uS temp = timer_hw->timerawh;
  return (temp << 32) | timer_hw->timerawl;
}

class softTimer {
  private:
    time_uS startTime;
    time_uS delay_uS;
    bool running;
    bool finishNow;
  public:
    softTimer() {
      startTime = 0;
      delay_uS = 0;
      running = false;
      finishNow = false;
    };
    void start(time_uS _delay_uS, time_uS _defer_uS) {
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
    time_uS getStartTime() {
      return startTime;  
    }
    time_uS getElapsed() {
      time_uS temp = getTheCurrentTime();
      return (temp < startTime ? 0 : temp - startTime);
    }
    time_uS getRemaining() {
      if (running) {
        time_uS temp = getElapsed();
        if (finishNow || (temp >= delay_uS)) {
          return 0;
        } else {
          return (delay_uS - temp);
        }
      } else {
        return 0;
      }  
    }
    time_uS getDelay()  {
      return delay_uS;
    }
};
