#pragma once

#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>


class TimerMt
{
public:
    TimerMt(const std::function<void(void)>& timeout);
    TimerMt(const std::function<void(void)>& timeout,
        const std::chrono::milliseconds& interval,
        bool singleShot = true);

    ~TimerMt();

    void start(bool multiThread = false);
    void stop();

    void setSingleShot(bool singleShot);
    bool isSingleShot() const;

    void setInterval(const std::chrono::milliseconds& interval);
    const std::chrono::milliseconds& interval() const;

    void setTimeout(const std::function<void(void)>& timeout);
    const std::function<void(void)>& timeout() const;

private:
    std::thread mThread;

    bool mIsSingleShot{ true };

    std::chrono::milliseconds mInterval{ 0 };
    std::function<void(void)> mTimeout{ nullptr };

    void temporize();

    bool mStopped{ true };
    std::mutex mMutex;
    std::condition_variable mTerminate;
};


TimerMt::TimerMt(const std::function<void(void)>& timeout)
    : mTimeout(timeout)
{
}

TimerMt::TimerMt(const std::function<void(void)>& timeout,
    const std::chrono::milliseconds& interval,
    bool singleShot)
    : mIsSingleShot(singleShot),
    mInterval(interval),
    mTimeout(timeout)
{
}

TimerMt::~TimerMt()
{
    stop();
}

void TimerMt::start(bool multiThread)
{
    stop();
    {
        auto locked = std::unique_lock<std::mutex>(mMutex);
        mStopped = false;
    }

    if (multiThread == true) 
    {
        mThread = std::thread(
            &TimerMt::temporize, this);
    }
    else 
    {
        this->temporize();
    }
}

void TimerMt::stop()
{
    {
        // Set the predicate
        auto locked = std::unique_lock<std::mutex>(mMutex);
        mStopped = true;
    }

    // Tell the thread the predicate has changed
    mTerminate.notify_one();

    if (mThread.joinable())
    {
        mThread.join();
    }
}


void TimerMt::setSingleShot(bool singleShot)
{
    mIsSingleShot = singleShot;
}

bool TimerMt::isSingleShot() const
{
    return mIsSingleShot;
}

void TimerMt::setInterval(const std::chrono::milliseconds& interval)
{
    if (mStopped == false)
        return;

    mInterval = interval;
}

const std::chrono::milliseconds& TimerMt::interval() const
{
    return mInterval;
}

void TimerMt::setTimeout(const std::function<void(void)>& timeout)
{
    if (mStopped == false)
        return;

    mTimeout = timeout;
}

const std::function<void(void)>& TimerMt::timeout() const
{
    return mTimeout;
}

void TimerMt::temporize()
{
    auto locked = std::unique_lock<std::mutex>(mMutex);

    while (!mStopped) // We hold the mutex that protects stop
    {
        auto result = mTerminate.wait_for(locked, mInterval);

        if (result == std::cv_status::timeout)
        {
            this->timeout()();
        }
        if (mIsSingleShot == true) 
        {
            stop();
        }
    }
}
