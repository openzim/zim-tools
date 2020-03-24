#include <functional>
#include <thread>
#include <mutex>
#include <list>
#include <condition_variable>
#include <sstream>

using namespace std;

class ZimThreadPool
{
    public:
    ZimThreadPool () :
    m_stop(false)
    {
        for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++)
        {
            auto sync_code = [this]()
            {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx);

                        cv.wait(lock, [this]{ return m_stop || !tasklist.empty(); });

                        if (!tasklist.empty())
                        {
                            task = tasklist.front();
                            tasklist.pop_front();
                        }
                        else
                        {
                            if (tasklist.empty())
                                return ;
                            task = [](){};
                        }
                    }
                    task();
                }
            };
            workers.emplace_back(std::move(sync_code));
        }
    }

    void addWork(std::function<void()> task)
    {
        std::unique_lock<std::mutex> lock(mtx);
        tasklist.push_back(task);
        cv.notify_one();
    }

    ~ZimThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            m_stop = true;
        }
        cv.notify_all();
        for (auto &worker : workers)
            worker.join();
    }

    private:
    std::mutex mtx;
    std::vector<std::thread> workers;
    std::list<std::function<void()>> tasklist;
    std::condition_variable cv;
    bool m_stop;
};
