//THE MOST BASIC VERSION OF STM. IMPLIMENTED IT OWN OUR OWN WITH WHAT I FOUND FIT USING A PSEUDO CODE I FOUND IN THE VERY FIRST PAPER 
//WE READ

#pragma GCC optimize("O3", "unroll-loops")

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

using namespace std;

class STM 
{
public:

    atomic<int> data[2];
    atomic<int> version;

    STM() 
    {
        data[0] = 0;
        data[1] = 0;
        version = 0;
    }

    bool compareAndSwapTransaction(int expected0, int expected1, int new0, int new1) 
    {
        int ver = version.load(memory_order_acquire);

        int d0 = data[0].load(memory_order_acquire);
        int d1 = data[1].load(memory_order_acquire);

        if(d0 != expected0 || d1 != expected1) 
        {
            return false; 
        }

        if(!version.compare_exchange_strong(ver, ver + 1, memory_order_acq_rel)) 
        {
            return false;
        }

        data[0].store(new0, memory_order_release);
        data[1].store(new1, memory_order_release);

        version.fetch_add(1, memory_order_acq_rel);

        return true;
    }
};

int main(int argc, char *argv[]) 
{
    int numThreads = stoi(argv[1]);

    STM stm;

    atomic<bool> stop{false};
    atomic<long long> totalOps{0};

    auto worker = [&]() 
    {
        while(!stop.load(memory_order_relaxed)) 
        {
            int d0 = stm.data[0].load(memory_order_acquire);
            int d1 = stm.data[1].load(memory_order_acquire);

            if(stm.compareAndSwapTransaction(d0, d1, d0 + 1, d1 + 1)) 
            {
                totalOps.fetch_add(1, memory_order_relaxed);
            }
        }
    };

    vector<thread> threads;

    for(int i = 0; i < numThreads; ++i) 
    {
        threads.emplace_back(worker);
    }

    auto startTime = chrono::high_resolution_clock::now();

    this_thread::sleep_for(chrono::seconds(5));

    stop.store(true, memory_order_relaxed);

    for(auto& t : threads) 
    {
        t.join();
    }

    auto endTime = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = endTime - startTime;

    int finalD0 = stm.data[0].load(memory_order_acquire);
    int finalD1 = stm.data[1].load(memory_order_acquire);

    int finalVersion = stm.version.load(memory_order_acquire);

    cout << "Total successful transactions: " << totalOps.load() << endl;

    cout << "Final STM state:" << endl;

    cout << "  Data[0] = " << finalD0 << endl;
    cout << "  Data[1] = " << finalD1 << endl;

    cout << "  Version = " << finalVersion << endl;

    cout << "Time taken: " << elapsed.count() << " seconds" << endl;
    
    cout << "Transactions per second: " << totalOps.load() / elapsed.count() << endl;

    return 0;
}
