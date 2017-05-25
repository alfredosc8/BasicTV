#include "thread.h"
#include "main.h"
#include "lock.h"

static lock_t lock;
static std::vector<std::pair<std::thread::id, thread_flags_t> > thread_list;

void thread_api::add(std::thread::id thread_id, thread_flags_t flags){
	thread_list.push_back(
		std::make_pair(
			thread_id,
			flags));
}

void thread_api::set_thread_flags(
	std::thread::id thread_id,
	thread_flags_t flags){
	for(uint64_t i = 0;i < thread_list.size();i++){
		if(thread_list[i].first == thread_id){
			thread_list[i].second = flags;
		}
	}
}

thread_flags_t thread_api::get_thread_flags(
	std::thread::id thread_id){
	for(uint64_t i = 0;i < thread_list.size();i++){
		if(thread_list[i].first == thread_id){
			return thread_list[i].second;
		}
	}
	return 0;
}
