#include <chrono>
#include <future>
#include <iostream>
#include <syncstream>
#include <string>
#include <thread>
#include <utility>

using namespace std;
using namespace std::chrono_literals;

void task(const string& name, chrono::seconds dur) {
    this_thread::sleep_for(dur);
    osyncstream sync_out(cout);
    sync_out << "Completed: " << name << '\n';
}


void chain_B1_D(shared_future<void> f_B2) {
    task("B1 (slow) depends on A1", 7s);
    f_B2.wait();
    task("D (quick) depends on B1,B2", 1s);
}

void work() {
    using clock = chrono::steady_clock;
    auto t0 = clock::now();

    osyncstream sync_out_start(cout);
    sync_out_start << "--- Starting Work (Variant 12) ---" << '\n';
    sync_out_start.emit();

    auto future_A1_status = async(launch::async, task, "A1 (slow)", 7s);

    auto future_A2_status = async(launch::async, task, "A2 (quick)", 1s);

    future_A2_status.wait();

    promise<void> p_b2;
    shared_future<void> f_b2 = p_b2.get_future().share();

    auto future_B2 = async(launch::async, [&p_b2]() {task("B2 (quick) depends on A2", 1s);p_b2.set_value();});

    auto future_C2 = async(launch::async, task, "C2 (quick) depends on A2", 1s);

    future_A1_status.wait();

    auto future_B1_D = async(launch::async, chain_B1_D, f_b2);

    auto future_C1 = async(launch::async, task, "C1 (quick) depends on A1", 1s);

    future_B1_D.get();

    future_B2.wait();
    future_C1.wait();
    future_C2.wait();

    auto t1 = clock::now();
    chrono::duration<double> duration = t1 - t0;
    {
        osyncstream sync_out_end(cout);
        sync_out_end << "--- All calculations completed ---" << '\n';
        sync_out_end << "Execution time: " << duration.count() << " seconds" << '\n';
    }
}

int main() {
    work();
    return 0;
}