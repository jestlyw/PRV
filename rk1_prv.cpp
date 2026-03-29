#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <queue>
#include <chrono>

using namespace std;

// ЗАДАЧА 1

mutex m1;
condition_variable cv1;
atomic<int> stage(0);

void stageWork(int id)
{
    unique_lock<mutex> lock(m1);

    while (stage != id)
    {
        cv1.wait(lock);
    }

    cout << "Stage " << id + 1 << " start\n";

    lock.unlock();
    this_thread::sleep_for(chrono::seconds(1));
    lock.lock();

    cout << "Stage " << id + 1 << " end\n";

    stage++;
    cv1.notify_all();
}

// ЗАДАЧА 2

mutex m2;
queue<int> q1, q2;
int sem1 = 0;
int sem2 = 0;

void worker(int id)
{
    while (true)
    {
        int task = -1;

        {
            lock_guard<mutex> lock(m2);

            if (sem1 > 0)
            {
                task = q1.front();
                q1.pop();
                sem1--;
            }
            else if (sem2 > 0)
            {
                task = q2.front();
                q2.pop();
                sem2--;
            }
            else
            {
                break;
            }
        }

        cout << "Worker " << id << " do task " << task << endl;
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

int main()
{
    int choice;
    cout << "1 - Task 1, 2 - Task 2: ";
    cin >> choice;

    if (choice == 1)
    {
        cout << "Task 1:\n";

        vector<thread> t;

        for (int i = 0; i < 4; i++)
        {
            t.push_back(thread(stageWork, i));
        }

        for (int i = 0; i < 4; i++)
        {
            t[i].join();
        }
    }
    else if (choice == 2)
    {
        cout << "Task 2:\n";

        for (int i = 1; i <= 5; i++)
        {
            q1.push(i);
            sem1++;
        }

        for (int i = 6; i <= 10; i++)
        {
            q2.push(i);
            sem2++;
        }

        vector<thread> t;

        for (int i = 0; i < 5; i++)
        {
            t.push_back(thread(worker, i));
        }

        for (int i = 0; i < 5; i++)
        {
            t[i].join();
        }
    }

    return 0;
}