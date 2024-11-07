// this worked on Nov 7 2024
// tested on binji.github.io/wasm-clang

// task manager takes a list of repeat periods and callback functions
// these should run in the background
// set an alarm to run the "on_irq" boolean every "tick" microseconds.
// load in the background functions you want using the "bind" language.
#include <iostream>
#include <vector>
#include <functional>

class task_mgr_obj {
    private:
        struct task_obj {
            int counter = 0;
            int period;
            std::function<void()> exec_on_trigger;
            void set_period(int arg_period) {
                period = arg_period;
            }
            void set_trigger(std::function<void()> arg_func) {
                exec_on_trigger = arg_func;
            }
            void increment(int add_uS) {
                std::cout << " incr " << counter << "+" << add_uS;
                counter += add_uS;
                std::cout << "=" << counter;
            }
            void execute() {
                exec_on_trigger();
            }
            bool triggered() {
                std::cout << " chk " << counter << ".vs." << period << "; ";
                if (counter >= period) {
                    counter = counter % period;
                    return true;
                }
                return false;
            }
        };
        int tick_uS = 0;
    public:
        task_mgr_obj(int arg_uS) {
            tick_uS = arg_uS;
        }
        std::vector<task_obj> task_list;
        int get_tick_uS() {
            return tick_uS;
        }
        void add_task(int arg_repeat_uS, std::function<void()> arg_on_trigger) {
            task_obj new_task;
            new_task.set_period(arg_repeat_uS);
            new_task.set_trigger(arg_on_trigger);
            task_list.emplace_back(new_task);
        }
};

task_mgr_obj task_mgr(8);

static bool on_irq() {
    std::cout << "in IRQ";
    int t = task_mgr.get_tick_uS();
    for (auto &i : task_mgr.task_list) {
        i.increment(t);
    }
    for (auto &i : task_mgr.task_list) {
        if (i.triggered()) {
            i.execute();
            return true;
        }
    }
    return false;
}

// pretend these are three different hardware drivers
class A {
    private:
        int repeat_uS = 32;
    public:
        int get_repeat() {
            return repeat_uS;
        }
        void doWork() {
            std::cout << " A:synth. ";
        }
};
class B {
    private:
        int repeat_uS = 256;
    public:
        int get_repeat() {
            return repeat_uS;
        }
        void doWork() {
            std::cout << " B:rotary. ";
        }
};
class C {
    private:
        int repeat_uS = 16;
    public:
        int get_repeat() {
            return repeat_uS;
        }
        void doWork() {
            std::cout << " C:hexes. ";
        }
};
A routine_A;
B routine_B;
C routine_C;
// 
// in Arduino environment, use this to start the background timer:
// #include "pico/stdlib.h"
// #include "pico/time.h"
// const uint frequency_poll    =    16;
// const uint rotary_rate_in_uS =   512;
// constexpr uint audio_sample_in_uS = 2 * frequency_poll;
// constexpr uint audio_sample_rate = 1'000'000 / audio_sample_in_uS;
// struct repeating_timer io_timer;

void setup_tasks() {
    task_mgr.add_task(routine_A.get_repeat(), std::bind(&A::doWork, &routine_A));
    task_mgr.add_task(routine_B.get_repeat(), std::bind(&B::doWork, &routine_B));
    task_mgr.add_task(routine_C.get_repeat(), std::bind(&C::doWork, &routine_C));
//   add_repeating_timer_us(frequency_poll, on_irq, NULL, &io_timer);
}

// in Arduino environment, run in setup1()
int main() {
    setup_tasks();
    // for testing in a c++ environment, pretend this loop runs every few microseconds
    int i = 0;
    for (int j = 0; j < 72; j++) {
        if (on_irq()) {
            i++;
        }
        std::cout << i << "/" << j  << "\n";
    }
    return i;
}
