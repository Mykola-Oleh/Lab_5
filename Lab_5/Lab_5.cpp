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

void work() {
    using clock = chrono::steady_clock;
    auto t0 = clock::now();
    {
        osyncstream sync_out_start(cout);
        sync_out_start << "--- Starting Work (Variant 12) ---" << '\n';
    }
    cout.flush();

    promise<void> p_A1;
    shared_future<void> f_A1 = p_A1.get_future().share();

    promise<void> p_B2;
    shared_future<void> f_B2 = p_B2.get_future().share();

    auto future_A1 = async(launch::async, [&p_A1]() {
        task("A1 (slow)", 7s);
        p_A1.set_value();
        });

    auto future_A2_chain = async(launch::async, [f_A1, &p_B2]() {
        task("A2 (quick)", 1s);

        task("B2 (quick) depends on A2", 1s);
        p_B2.set_value();

        task("C2 (quick) depends on A2", 1s);

        f_A1.wait();

        task("C1 (quick) depends on A1", 1s);
        });


    auto future_B1_D = async(launch::async, [f_A1, f_B2]() {
        f_A1.wait();

        task("B1 (slow) depends on A1", 7s);

        f_B2.wait();

        task("D (quick) depends on B1,B2", 1s);
        });

    future_B1_D.get();

    future_A1.wait();
    future_A2_chain.wait();

    auto t1 = clock::now();
    chrono::duration<double> duration = t1 - t0;

    {
        osyncstream sync_out_end(cout);
        sync_out_end << "--- All calculations completed ---" << '\n';
        sync_out_end << "Execution time: " << duration.count() << " seconds" << '\n';
        sync_out_end << "Work is done!\n";
    }
}

int main() {
    work();
    return 0;
}