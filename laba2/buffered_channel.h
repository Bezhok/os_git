#ifndef BUFFERED_CHANNEL_H_
#define BUFFERED_CHANNEL_H_

#include <thread>
#include <chrono>
#include <mutex>
#include <utility>
#include <queue>
#include <condition_variable>
#include <atomic>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : buffer_size_(size) {
        is_closed_ = false;
    }

    void Send(T value) {
        std::unique_lock<std::mutex> locker(common_mutex_);

        if (buffer_size_ == queue_.size())
            overflow__check.wait(locker, [&]() {
                return buffer_size_ != queue_.size() || is_closed_;
            });

        if (is_closed_) {
            // maybe should store in vector?
            // return;
            throw std::runtime_error("closed");
        }

        queue_.push(std::move(value));
        empty__check.notify_one();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> locker(common_mutex_);

        if (queue_.empty())
            empty__check.wait(locker, [&]() {
                return !queue_.empty() || is_closed_;
            });

        if (is_closed_ && queue_.empty()) {
            return {T(), false};
        }

        std::pair<T, bool> buff = {queue_.front(), true};
        queue_.pop();
        overflow__check.notify_one();
        return buff;
    }

    void Close() {
        common_mutex_.lock();
        is_closed_ = true;
        common_mutex_.unlock();

        empty__check.notify_all();
        overflow__check.notify_all();
    }

private:
    std::condition_variable overflow__check;
    std::condition_variable empty__check;
    std::atomic_bool is_closed_;
    uint32_t buffer_size_;
    std::queue<T> queue_;
    std::mutex common_mutex_;
};

#endif // BUFFERED_CHANNEL_H_