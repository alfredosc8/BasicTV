#include "stats.h"

/*
  ALL return values that are not P-values are variable length numbers
  (have to be decoded through math::number::get...). Even though most
  statistics functions' outputs don't use units, I think that larger sample sets
  would benefit (namely in intermediate operations).
 */

