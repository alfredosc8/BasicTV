#ifndef THREAD_H
#define THREAD_H

#include "thread"
#include "mutex"

/*
  Any and all things threading go in here

  I am making this file not only for multiple threads at one time, but to also
  create a state system for each thread (including the main) that prevents
  infinite loops with ID lookups.
 */

typedef uint16_t thread_flags_t;

namespace thread_api{
	void add(
		std::thread::id thread_id,
		thread_flags_t flags);
	void set_thread_flags(
		std::thread::id thread_id,
		thread_flags_t flags);
	thread_flags_t get_thread_flags(
		std::thread::id thread_id);
};

#endif
