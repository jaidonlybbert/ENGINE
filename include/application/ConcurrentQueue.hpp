#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ConcurrentQueue
{
public:
	void push(T item)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(std::move(item));
		m_cv.notify_one();
	}

	T pop()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return !m_queue.empty(); });

		T item = std::move(m_queue.front());
		m_queue.pop();
		return item;
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_queue;
};
