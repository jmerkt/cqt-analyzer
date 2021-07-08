#include "TimerMt.h"

namespace multithreading {

    TimerMt::TimerMt(const Timeout& timeout)
        : _timeout(timeout)
    {
    }

    TimerMt::TimerMt(const TimerMt::Timeout& timeout,
        const TimerMt::Interval& interval,
        bool singleShot)
        : _isSingleShot(singleShot),
        _interval(interval),
        _timeout(timeout)
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
            auto locked = std::unique_lock<std::mutex>(_mutex);
            _stopped = false;
        }

        if (multiThread == true) {
            _thread = std::thread(
                &TimerMt::_temporize, this);
        }
        else {
            this->_temporize();
        }
    }

    void TimerMt::stop()
    {
        {
            // Set the predicate
            auto locked = std::unique_lock<std::mutex>(_mutex);
            _stopped = true;
        }

        // Tell the thread the predicate has changed
        _terminate.notify_one();

        if (_thread.joinable())
        {
            _thread.join();
        }
    }


    void TimerMt::setSingleShot(bool singleShot)
    {
        _isSingleShot = singleShot;
    }

    bool TimerMt::isSingleShot() const
    {
        return _isSingleShot;
    }

    void TimerMt::setInterval(const TimerMt::Interval& interval)
    {
        if (_stopped == false)
            return;

        _interval = interval;
    }

    const TimerMt::Interval& TimerMt::interval() const
    {
        return _interval;
    }

    void TimerMt::setTimeout(const Timeout& timeout)
    {
        if (_stopped == false)
            return;

        _timeout = timeout;
    }

    const TimerMt::Timeout& TimerMt::timeout() const
    {
        return _timeout;
    }

    void TimerMt::_temporize()
    {
        auto locked = std::unique_lock<std::mutex>(_mutex);

        while (!_stopped) // We hold the mutex that protects stop
        {
            auto result = _terminate.wait_for(locked, _interval);

            if (result == std::cv_status::timeout)
            {
                this->timeout()();
            }
            if (_isSingleShot == true) {
                stop();
            }
        }
    }
}