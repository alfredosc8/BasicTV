#ifndef ID_SET_H
#define ID_SET_H
#include "id.h"
#include "../util.h"
/*
  This is for large sets of IDs that should only be referenced once

  This assumes that at least two IDs are from the same client (and thus, the
  same hash), and it just lists this down
 */

std::vector<id_t_> expand_id_set(std::vector<uint8_t> id_set);
std::vector<uint8_t> compact_id_set(std::vector<id_t_> id_set);
#endif
