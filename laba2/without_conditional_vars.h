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
        size_ = 0u;
    }

    void Send(T value) {
        std::unique_lock<std::mutex> locker(send_mutex_);
        if (buffer_size_ == size_) {
            while (buffer_size_ == size_) {
                if (size_ == buffer_size_ && is_closed_) {
                    throw std::runtime_error("closed");
                }
            }
        }

        common_mutex_.lock();
        if (is_closed_) {
            common_mutex_.unlock();
            throw std::runtime_error("closed");
            return;
        }

        queue_.push(std::move(value));
        size_ = queue_.size();

        common_mutex_.unlock();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> locker(receive_mutex_);

        if (size_ == 0u) {
            while (size_ == 0) {
                if (size_ == 0 && is_closed_) {
                    return {T(), false};
                }
            }
        }

        common_mutex_.lock();
        if (is_closed_ && size_ == 0) {
            common_mutex_.unlock();
            return {T(), false};
        }

        std::pair<T, bool> buff = {queue_.front(), true};
        queue_.pop();
        size_ = queue_.size();
        common_mutex_.unlock();
        return buff;
    }

    void Close() {
        common_mutex_.lock();
        is_closed_ = true;
        common_mutex_.unlock();
    }

private:
    std::atomic_uint32_t size_;
    std::atomic_bool is_closed_;
    uint32_t buffer_size_;
    std::queue<T> queue_;
    std::mutex receive_mutex_;
    std::mutex send_mutex_;
    std::mutex common_mutex_;
};

#endif // BUFFERED_CHANNEL_H_