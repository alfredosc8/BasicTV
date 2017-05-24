#ifndef LOOP_H
#define LOOP_H

#include "main.h"

#include "settings.h"
#include "util.h"
#include "id/id.h"

/*
  Contains whatever code I need that pertains to general execution
  and looping

  This code is pretty rag-tag, and should probably be changed. However,
  this isn't that important and if it fails it isn't a big deal.
 */

// set time or iterations
extern void check_finite_execution_modes(
	uint64_t, uint64_t);

// print after every iteration, slow down execution, etc.
extern void check_iteration_modifiers();

// print stats
extern void check_print_modifiers(
	uint64_t avg_iter_time);

extern void update_iteration_data(
	uint64_t*, uint64_t*, uint64_t*);

#endif
