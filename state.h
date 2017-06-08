#ifndef STATE_H
#define STATE_H

#include "util.h"

/*
  Generic state interface for multiple functions

  Probably the simplest file in the entire program
 */

template <typename T>
void state_sanity_check(T state){
	if(state == nullptr){
		print("state is a nullptr", P_ERR);
	}
	if(state->get_state_ptr() == nullptr){
		print("state_ptr is a nullptr",P_ERR);
	}
}


struct state_t{
private:
	void *state_ptr = nullptr;
	uint64_t state_format = 0;
public:
	GET_SET(state_ptr, void*);
	GET_SET(state_format, uint64_t);
};

#endif
