#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <mutex>
#include <unordered_map>

using namespace std;

atomic<int> global_clock{0};
mutex commit_lock;

class STMObject 
{
public:
    STMObject(int init) : value(init), version(0) {}

    atomic<int> value;
    atomic<int> version;
};

class Transaction 
{
public:
    Transaction() : aborted(false) 
    {
        start_time = global_clock.load(memory_order_acquire);
        snapshot_upper = start_time;
    }
    
    int read(STMObject* obj) 
    {
        int obj_ver = obj->version.load(memory_order_acquire);

        while(obj_ver > snapshot_upper) 
        {
            int new_time = global_clock.load(memory_order_acquire);

            if(new_time == snapshot_upper) 
            {
                aborted = true;

                return -1;
            }

            snapshot_upper = new_time;

            obj_ver = obj->version.load(memory_order_acquire);
        }

        readSet[obj] = obj_ver;

        return obj->value.load(memory_order_acquire);
    }
    
    void write(STMObject* obj, int newValue) 
    {
        writeSet[obj] = newValue;
    }
    
    bool commit() 
    {
        lock_guard<mutex> lock(commit_lock);

        for(auto& entry : readSet) 
        {
            STMObject* obj = entry.first;

            int expected = entry.second;
            int current_ver = obj->version.load(memory_order_acquire);

            if(current_ver != expected || current_ver > snapshot_upper) 
            {
                return false;
            }
        }

        int commit_time = global_clock.fetch_add(1, memory_order_acq_rel) + 1;

        if(commit_time - 1 > snapshot_upper) 
        {
            return false;
        }

        for(auto& entry : writeSet) 
        {
            STMObject* obj = entry.first;

            obj->value.store(entry.second, memory_order_release);
            obj->version.store(commit_time, memory_order_release);
        }

        return true;
    }
    
    bool isAborted() const { return aborted; }
    
private:
    int start_time;
    int snapshot_upper;

    bool aborted;

    unordered_map<STMObject*, int> readSet;
    unordered_map<STMObject*, int> writeSet;
};

void transactionalWork(int threadId, vector<STMObject*>& objects, int numTransactions) 
{
    for(int i = 0; i < numTransactions; i++) 
    {
        bool committed = false;

        while (!committed) 
        {
            Transaction txn;

            bool abort_flag = false;

            for(auto& obj : objects) 
            {
                int val = txn.read(obj);

                if(txn.isAborted())
                {
                    abort_flag = true;

                    break;
                }

                txn.write(obj, val + 1);
            }

            if(abort_flag) 
            {
                this_thread::yield();

                continue;
            }
            if(txn.commit()) 
            {
                committed = true;
            } 
            else 
            {
                this_thread::yield();
            }
        }
    }

    lock_guard<mutex> cout_lock(commit_lock);
    cout << "Thread " << threadId << " finished." << endl;
}

int main() 
{
    int numThreads = thread::hardware_concurrency();

    int numSTMObjects = 2;

    vector<STMObject*> objects;

    for(int i = 0; i < numSTMObjects; i++) 
    {
        objects.push_back(new STMObject(0));
    }
    
    const int transactionsPerThread = 1000;

    vector<thread> threads;

    auto startTime = chrono::high_resolution_clock::now();

    for(int i = 0; i < numThreads; i++) 
    {
        threads.emplace_back(transactionalWork, i, ref(objects), transactionsPerThread);
    }
    for(auto& t : threads) 
    {
        t.join();
    }

    auto endTime = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = endTime - startTime;
    
    cout << "\nFinal STM state:\n";

    for(size_t i = 0; i < objects.size(); i++) 
    {
        int final_val = objects[i]->value.load(memory_order_acquire);

        int final_ver = objects[i]->version.load(memory_order_acquire);

        cout << "  Object " << i << ": value = " << final_val << ", version = " << final_ver << endl;
    }

    cout << "Time taken: " << elapsed.count() << " seconds\n";
    
    for(auto obj : objects) 
    {
        delete obj;
    }
    
    return 0;
}
