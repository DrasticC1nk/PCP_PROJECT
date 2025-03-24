#pragma GCC optimize("O3", "unroll-loops")

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>

using namespace std;    

class STMObject 
{
public:
    STMObject(int init) : value(init) {}

    atomic<int> value;
};

class GroupLock 
{
public:
    void lock() { mtx.lock(); }
    
    void unlock() { mtx.unlock(); }

private:
    mutex mtx;
};

class Transaction 
{
public:
    Transaction(GroupLock &lock) : groupLock(lock) { groupLock.lock(); }

    ~Transaction() { groupLock.unlock(); }

    int read(STMObject &obj) 
    {
        return obj.value.load();
    }

    void write(STMObject &obj, int newValue) 
    {
        obj.value.store(newValue);
    }

private:
    GroupLock &groupLock;
};

void transactionalWork(int threadId, STMObject &obj1, STMObject &obj2, GroupLock &lock, int numTransactions) 
{
    for(int i = 0; i < numTransactions; i++) 
    {
        Transaction txn(lock);

        int v1 = txn.read(obj1);
        int v2 = txn.read(obj2);

        txn.write(obj1, v1 + 1);
        txn.write(obj2, v2 + 1);
    }
}

int main(int argc, char *argv[]) 
{
    STMObject obj1(0), obj2(0);

    GroupLock lock;

    int transactionsPerThread = 1000;

    vector<thread> threads;
    
    auto start = chrono::high_resolution_clock::now();

    int numThreads = stoi(argv[1]);
    
    for(int i = 0; i < numThreads; i++) 
    {
        threads.emplace_back(transactionalWork, i, ref(obj1), ref(obj2), ref(lock), transactionsPerThread);
    }
    
    for(auto &t : threads) 
    {
        t.join();
    }
    
    auto end = chrono::high_resolution_clock::now();
    
    chrono::duration<double> elapsed = end - start;
    
    cout << "Final STM state:\n";
    cout << "  Object 0: value = " << obj1.value.load() << "\n";
    cout << "  Object 1: value = " << obj2.value.load() << "\n";
    cout << "Time taken: " << elapsed.count() << " seconds\n";
    
    return 0;
}
