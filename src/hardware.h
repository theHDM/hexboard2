#pragma once
#include <vector>
#include "syntacticSugar.h"
#include "config.h"             // contains pin and timing definitions
#include "timing.h"

#include "hwRotary.h"
#include "hwKeys.h"
#include "hwAudio.h"

#include "hardware/irq.h"       // library of code to let you interrupt code execution to run something of higher priority


constexpr int target_audio_sample_halfperiod_in_uS = 500'000 / target_audio_sample_rate_in_Hz;
constexpr int hardware_tick_period_in_uS = 
  ( target_audio_sample_halfperiod_in_uS > keyboard_pin_reset_period_in_uS
  ? target_audio_sample_halfperiod_in_uS : keyboard_pin_reset_period_in_uS ); 
constexpr int actual_audio_sample_period_in_uS = 2 * hardware_tick_period_in_uS;
constexpr int actual_audio_sample_rate_in_Hz  = 1'000'000 / actual_audio_sample_period_in_uS;

// this worked on Nov 7 2024
// tested on binji.github.io/wasm-clang

// task manager takes a list of repeat periods and callback functions
// these should run in the background
// set an alarm to run the "on_irq" boolean every "tick" microseconds.
// load in the background functions you want using the "bind" language.

static void on_irq();

class task_mgr_obj {
    private:
        struct task_obj {
            int counter = 0;
            int period;
            func exec_on_trigger;
            void set_period(int arg_period) {
                period = arg_period;
            }
            void set_trigger(func arg_func) {
                exec_on_trigger = arg_func;
            }
            void increment(int add_uS) {
                counter += add_uS;
            }
            void execute() {
                exec_on_trigger();
            }
            bool triggered() {
                if (counter >= period) {
                    counter = counter % period;
                    return true;
                }
                return false;
            }
        };
        int tick_uS = 0;
        byte alarm_ID = 0;
    public:
        task_mgr_obj(int arg_uS) {
            tick_uS = arg_uS;
        }
        std::vector<task_obj> task_list;
        int get_tick_uS() {
            return tick_uS;
        }
        void add_task(int arg_repeat_uS, func arg_on_trigger) {
            task_obj new_task;
            new_task.set_period(arg_repeat_uS);
            new_task.set_trigger(arg_on_trigger);
            task_list.emplace_back(new_task);
        }
        void set_timer() {
          timer_hw->alarm[alarm_ID] = getTheCurrentTime() + tick_uS;
        }
        void begin() {
          hw_set_bits(&timer_hw->inte, 1u << alarm_ID);  // initialize the timer
          irq_set_exclusive_handler(alarm_ID, on_irq);     // function to run every interrupt
          irq_set_enabled(alarm_ID, true);               // ENGAGE!
          set_timer();
        }
        void repeat_timer() {
          hw_clear_bits(&timer_hw->intr, 1u << alarm_ID);
          set_timer();
        }
};

// global define
task_mgr_obj task_mgr(hardware_tick_period_in_uS);

// global routine
static void on_irq() {
    task_mgr.repeat_timer();
    int t = task_mgr.get_tick_uS();
    for (auto &i : task_mgr.task_list) {
        i.increment(t);
    }
    for (auto &i : task_mgr.task_list) {
        if (i.triggered()) {
            i.execute();
            break;
        }
    }
}

// global, call this on the main core
void setupDevices() {
    setupAudio(pwmPins);
    rotary.setup(rotaryPinA,rotaryPinB,rotaryPinC);
    pinGrid.setup(colPins, muxPins, true); // true = iterate mux pins first, false = iterate col pins first
}

// global, call this on the background core
void setupTaskMgr() {
    // add highest priority tasks first.
    task_mgr.add_task(actual_audio_sample_period_in_uS, audioPoll);
    task_mgr.add_task(rotary_pin_fire_period_in_uS,     std::bind(&rotary_obj::poll, &rotary));
    task_mgr.add_task(keyboard_pin_reset_period_in_uS,  std::bind(&pinGrid_obj::poll, &pinGrid));
    task_mgr.begin();
}

// two task objects
// take alarm num as a variable
// see if it works, eh
