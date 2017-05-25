#ifndef LOCK_H
#define LOCK_H
#include <mutex>
#include <thread>
struct lock_t{
private:
	std::mutex mutex_lock;
	std::thread::id id;
	uint16_t depth = 0;
public:
	lock_t();
	~lock_t();
	void lock();
	void unlock();
};
#endif
