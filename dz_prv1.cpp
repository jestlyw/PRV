#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <ctime>

using namespace std;

vector<vector<int>> a;
long long sum = 0;
mutex m;
condition_variable cv;
int done = 0;

void calc(int l, int r)
{
    long long s = 0;

    for (int i = l; i < r; i++)
    {
        for (int j = 0; j < a[i].size(); j++)
        {
            s += a[i][j];
        }
    }

    {
        lock_guard<mutex> lock(m);
        sum += s;
        done++;
    }

    cv.notify_one();
}

int main()
{
    int n = 1000, mcol = 1000;
    int k = 4;

    a.resize(n, vector<int>(mcol));

    srand(time(0));

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < mcol; j++)
        {
            a[i][j] = rand() % 10 + 1;
        }
    }

    auto t1 = chrono::high_resolution_clock::now();

    vector<thread> th;
    int part = n / k;

    for (int i = 0; i < k; i++)
    {
        int l = i * part;
        int r = (i == k - 1) ? n : l + part;
        th.push_back(thread(calc, l, r));
    }

    {
        unique_lock<mutex> lock(m);
        cv.wait(lock, [&] { return done == k; });
    }

    for (int i = 0; i < th.size(); i++)
    {
        th[i].join();
    }

    auto t2 = chrono::high_resolution_clock::now();
    auto t = chrono::duration_cast<chrono::milliseconds>(t2 - t1);

    cout << "Sum: " << sum << endl;
    cout << "Time: " << t.count() << " ms" << endl;

    return 0;
}