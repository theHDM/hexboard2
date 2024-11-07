// this worked on Nov 7 2024
// tested on binji.github.io/wasm-clang

// task manager takes a list of repeat periods and callback functions
// these should run in the background
// set an alarm to run the "on_irq" boolean every "tick" microseconds.
// load in the background functions you want using the "bind" language.
//

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
                std::cout << " incrementing " << counter << " by " << add_uS;
                counter += add_uS;
                std::cout << " now it's " << counter;
            }
            void execute() {
                exec_on_trigger();
            }
            bool triggered() {
                std::cout << " checking trigger ";
                std::cout << " counter is " << counter << " period is " << period << "; ";
                if (counter >= period) {
                    counter -= period;
                    return true;
                }
                return false;
            }
        };
        int tick_uS = 0;
    public:
        std::vector<task_obj> task_list;
        void set_tick_uS(int arg_uS) {
            tick_uS = arg_uS;
        }
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

class A {
    private:
        int repeat_uS = 1;
    public:
        void set_repeat(int arg_uS) {
            repeat_uS = arg_uS;
        }
        int get_repeat() {
            return repeat_uS;
        }
        void doWork() {
            std::cout << " A:didWork. ";
        }
};

task_mgr_obj task_mgr;

static bool on_irq() {
    std::cout << "in IRQ";
    int t = task_mgr.get_tick_uS();
    for (auto &i : task_mgr.task_list) {
        std::cout << ", incremented a task by " << t << "; ";
        i.increment(t);
    }
    for (auto &i : task_mgr.task_list) {
        if (i.triggered()) {
            std::cout << "triggered a task; ";
            i.execute();
            return true;
        }
    }
    return false;
}

int main() {
    task_mgr.set_tick_uS(24);
    A routine_A;
    routine_A.set_repeat(512);
    task_mgr.add_task(
        routine_A.get_repeat(), 
        std::bind(&A::doWork, &routine_A)
    );
    int i = 0;
    for (int j = 0; j < 100; j++) {
        if (on_irq()) {
            i++;
        }
        std::cout << "loop#" << j << " triggers=" << i << "\n";
    }
    return i;
}
