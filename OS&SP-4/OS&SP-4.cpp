#include <bits.h>
#include <chrono>
#include <windows.h>
#include <iostream>
#include <mutex>
#include <random>

#define ll long long
#define ld long double
#define vt vector<thread>
#define vm vector<mutex>

using namespace std;
using namespace chrono;

#define PHILOSOPHERS_COUNT 5
#define CONFLICT_STRATEGY ConflictResolutionStrategy::ResourceHierarchy //see enum

ll THINK_MIN = 50;
ll THINK_MAX = 1000;

ll EAT_MIN = 50;
ll EAT_MAX = 100;

#define SIMULATION_TIME 5

enum class ConflictResolutionStrategy
{
    ResourceHierarchy,
    Waiter
};

class Philosopher
{
    mutex& leftFork;
    mutex& rightFork;

    ConflictResolutionStrategy strategy;
    mutex& waiter;
    atomic<bool>& stopFlag;
    mt19937 random;

    uniform_int_distribution<int> thinkDistribution;
    uniform_int_distribution<int> eatDistribution;

    bool pickUpForks(mutex& firstFork, mutex& secondFork)
    {
        if (!firstFork.try_lock())
        {
            return false;
        }
        if (!secondFork.try_lock())
        {
            firstFork.unlock();
            return false;
        }
        return true;
    }

public:

    Philosopher(ll id, mutex& leftFork, mutex& rightFork,
        ConflictResolutionStrategy strategy,
        mutex& waiter, atomic<bool>& stopFlag,
        mt19937& rng,
        uniform_int_distribution<int>& thinkDistribution,
        uniform_int_distribution<int>& eatDistribution)

        : id(id), leftFork(leftFork), rightFork(rightFork),
        strategy(strategy),
        waiter(waiter), stopFlag(stopFlag),
        random(random),
        thinkDistribution(thinkDistribution),
        eatDistribution(eatDistribution),
        totalThinkingTime(0), totalEatingTime(0),
        successfulEats(0), failedAttempts(0) { }

    void eat()
    {
        while (!stopFlag.load())
        {
            auto thinkTime = thinkDistribution(random);

            auto startThink = steady_clock::now();
            this_thread::sleep_for(milliseconds(thinkTime));
            auto endThink = steady_clock::now();

            totalThinkingTime += duration_cast<milliseconds>(endThink - startThink).count();

            bool eaten = false;

            if (strategy == ConflictResolutionStrategy::ResourceHierarchy)
            {
                if (id % 2 == 0)
                {
                    eaten = pickUpForks(leftFork, rightFork);
                }
                else
                {
                    eaten = pickUpForks(rightFork, leftFork);
                }
            }
            else if (strategy == ConflictResolutionStrategy::Waiter)
            {
                unique_lock<mutex> lock(waiter);
                eaten = pickUpForks(leftFork, rightFork);
            }

            if (eaten)
            {
                auto eatTime = eatDistribution(random);

                auto startEat = steady_clock::now();
                this_thread::sleep_for(milliseconds(eatTime));
                auto endEat = steady_clock::now();

                totalEatingTime += duration_cast<milliseconds>(endEat - startEat).count();
                ++successfulEats;

                leftFork.unlock();
                rightFork.unlock();
            }
            else
            {
                ++failedAttempts;
            }
        }
    }

    ll totalThinkingTime;
    ll totalEatingTime;
    ll successfulEats;
    ll failedAttempts;

    ll id;
};

class Table
{
    ll philosophersCount;
    vector<Philosopher*> philosophers;

    vm forks;
    vt threads;

    uniform_int_distribution<int> thinkDistribution;
    uniform_int_distribution<int> eatDistribution;

    ConflictResolutionStrategy strategy;
    mt19937 random{ random_device{}() };
    ll simulationTime;
    atomic<bool> stopFlag;
    mutex waiter;

public:

    Table(int philosophersCount,
        ConflictResolutionStrategy strategy,
        int thinkMin, int thinkMax,
        int eatMin, int eatMax,
        int simulationTime)
        
        : philosophersCount(philosophersCount),
        strategy(strategy),
        thinkDistribution(thinkMin, thinkMax),
        eatDistribution(eatMin, eatMax),
        simulationTime(simulationTime),
        stopFlag(false), waiter()
    {
        forks = vm(philosophersCount);

        for (ll i = 0; i < philosophersCount; ++i)
        {
            philosophers.emplace_back(new Philosopher(
                i,
                forks[i],
                forks[(i + 1) % philosophersCount],
                strategy,
                waiter,
                stopFlag,
                random,
                thinkDistribution,
                eatDistribution));
        }
    }

    ~Table()
    {
        for (auto p : philosophers)
        {
            delete p;
        }
    }

    void startEat()
    {
        for (auto p : philosophers)
        {
            threads.emplace_back(&Philosopher::eat, p);
        }

        this_thread::sleep_for(seconds(simulationTime));

        stopFlag.store(true);

        for (auto& t : threads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

    void printStatistics() const
    {
        cout << "Modeling statistics:\n";

        ll totalThinking = 0;
        ll totalEating = 0;
        ll totalSuccess = 0;
        ll totalFailures = 0;

        for (const auto p : philosophers)
        {
            cout << "  Philosopher " << p->id << ":" << endl;
            cout << "    Think time: " << p->totalThinkingTime << " ms" << endl;
            cout << "    Eat time: " << p->totalEatingTime << " ms" << endl;
            cout << "    Successful eat count: " << p->successfulEats << endl;
            cout << "    Unsuccessful attempt count: " << p->failedAttempts << endl;

            totalThinking += p->totalThinkingTime;
            totalEating += p->totalEatingTime;
            totalSuccess += p->successfulEats;
            totalFailures += p->failedAttempts;
        }

        cout << endl << "General statistics:" << endl;
        cout << "  General think time: " << totalThinking << " ms" << endl;
        cout << "  General eat time: " << totalEating << " ms" << endl;
        cout << "  General successful eat count: " << totalSuccess << endl;
        cout << "  General Unsuccessful eat count: " << totalFailures << endl;
        cout << "  General efficiency (transmit): " << static_cast<ld>(totalSuccess) / simulationTime << " eat(s)/sec\n";
    }
};

int main()
{
    ConflictResolutionStrategy strategy;
    try
    {
        strategy = CONFLICT_STRATEGY;
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        return 1;
    }

    cout << "Philosophers count: " << PHILOSOPHERS_COUNT << endl;
    cout << "Conflict resolution strategy: " << (strategy == ConflictResolutionStrategy::ResourceHierarchy ? "ResourceHierarchy" : "Waiter") << endl;
    cout << "Think time: " << THINK_MIN << "-" << THINK_MAX << " ms" << endl;
    cout << "Eat time: " << EAT_MIN << "-" << EAT_MAX << " ms" << endl;
    cout << "Simulation time: " << SIMULATION_TIME << " seconds" << endl << endl;

    Table table(PHILOSOPHERS_COUNT, strategy, THINK_MIN, THINK_MAX, EAT_MIN, EAT_MAX, SIMULATION_TIME);
    table.startEat();
    table.printStatistics();

    return 0;
}