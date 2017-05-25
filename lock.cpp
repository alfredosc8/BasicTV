#include "util.h"
#include "main.h"
#include "lock.h"
#include "settings.h"

/*
  locking system doesn't work right, and since this is a singlethreaded
  program (ideally), then it doesn't make sense to use locks in the first
  place. Fix this when there is a need.
 */

lock_t::lock_t(){
}

lock_t::~lock_t(){
}

void lock_t::lock(){
	const std::thread::id this_id =
		std::this_thread::get_id();
	if(depth == 0){
		id = this_id;
	}
	if(id == this_id){
		depth++;
	}else{
		mutex_lock.lock();
	}
}

void lock_t::unlock(){
	const std::thread::id this_id =
		std::this_thread::get_id();
	if(depth == 0){
		return;
	}
	if(id == this_id){
		depth--;
		if(depth == 0){
			mutex_lock.unlock();
		}
	}
}
