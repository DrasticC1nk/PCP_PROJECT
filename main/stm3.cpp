//THIS IS OUR OWN IMPLIMENTATION WITH SOME OPTIMIZATIONS WE THOUGHT OF LIKE STORING THE VALUE AND VERSION PACKED INTO A SINGLE UNSIGNED 
//64 BIT INTEGER. THIS SHOULD IMPROVE CACHE CONTETION. SOME IF IT'S IDEA WAS FOM 24TH'S LECTURE WE WE LEARNED SOMETHING SIMILAR BEIING
//DONE TO THE LINKED LISTS. ALSO WE ADDED FINE GRAINED LOCKING WHICH CAN BE DONE BY NOT LOCKING ALL THE OBJECTS BT RAHTER PER OBJECT.
//HERE THE OBJECT IS BEING REFFERED TO AN STM OBJECT.

#pragma GCC optimize("O3", "unroll-loops")

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <string>

using namespace std;

class STMObject 
{
public:
    STMObject(int init) : state((static_cast<uint64_t>(0) << 32) | (static_cast<uint32_t>(init))) {}

    atomic<uint64_t> state;
};

mutex globalCommitLock;  
mutex coutMutex;         

inline uint32_t getValue(uint64_t state) 
{
    return static_cast<uint32_t>(state & 0xFFFFFFFF);
}

inline uint32_t getVersion(uint64_t state) 
{
    return static_cast<uint32_t>(state >> 32);
}

class Transaction 
{
public:
    int read(STMObject* obj) 
    {
        uint64_t state = obj->state.load();

        int value = static_cast<int>(getValue(state));

        uint32_t version = getVersion(state);

        readSet[obj] = version;

        return value;
    }

    void write(STMObject* obj, int newValue) 
    {
        writeSet[obj] = newValue;
    }

    bool commit() 
    {
        lock_guard<mutex> lock(globalCommitLock);

        for(auto &entry : readSet) 
        {
            STMObject* obj = entry.first;

            uint32_t recordedVersion = entry.second;

            if(writeSet.find(obj) != writeSet.end())
            {
                continue;
            }

            uint64_t currentState = obj->state.load();

            if(getVersion(currentState) != recordedVersion) 
            {
                return false;
            }
        }

        for(auto &entry : writeSet) 
        {
            STMObject* obj = entry.first;

            int newValue = entry.second;

            uint64_t currentState = obj->state.load();
            uint32_t recordedVersion = 0;

            if(readSet.find(obj) != readSet.end()) 
            {
                recordedVersion = readSet[obj];
            }
            else 
            {
                recordedVersion = getVersion(currentState);
            }

            if(getVersion(currentState) != recordedVersion) 
            {
                return false; 
            }

            uint32_t newVersion = recordedVersion + 1;
            uint64_t newState = (static_cast<uint64_t>(newVersion) << 32) | (static_cast<uint32_t>(newValue));

            obj->state.store(newState);
        }

        return true;
    }
    
private:
    unordered_map<STMObject*, uint32_t> readSet;
    unordered_map<STMObject*, int> writeSet;
};

void transactionalWork(int threadId, vector<unique_ptr<STMObject>>& objects, int numTransactions) 
{
    for(int i = 0; i < numTransactions; i++) 
    {
        bool committed = false;

        while(!committed)
        {
            Transaction txn;

            for(auto &objPtr : objects) 
            {
                int val = txn.read(objPtr.get());

                txn.write(objPtr.get(), val + 1);
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
    {
        lock_guard<mutex> lock(coutMutex);

        cout << "Thread " << threadId << " finished.\n";
    }
}

int main(int argc, char *argv[]) 
{
    int numThreads = stoi(argv[1]);

    int numSTMObjects = 2;

    vector<unique_ptr<STMObject>> objects;

    for(int i = 0; i < numSTMObjects; i++) 
    {
        objects.emplace_back(make_unique<STMObject>(0));
    }
    
    const int transactionsPerThread = 1000;

    vector<thread> threads;
    
    auto startTime = chrono::high_resolution_clock::now();
    
    for(int i = 0; i < numThreads; i++) 
    {
        threads.emplace_back(transactionalWork, i, ref(objects), transactionsPerThread);
    }
    
    for(auto &t : threads) 
    {
        t.join();
    }
    
    auto endTime = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed = endTime - startTime;
    
    {
        lock_guard<mutex> lock(coutMutex);
        
        for(size_t i = 0; i < objects.size(); i++) 
        {
            uint64_t state = objects[i]->state.load();
            cout << "Final value for object " << i << ": " << getValue(state) << endl;
        }
    }
    
    cout << "Time taken: " << elapsed.count() << " seconds\n";
    cout << "Transactions per second: " << (numThreads * transactionsPerThread) / elapsed.count() << "\n";
    
    return 0;
}
