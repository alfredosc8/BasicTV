#ifndef STATS_H
#define STATS_H
#include "../main.h"
#include "../util.h"

/*
  New approach:
  stat_sample_set_t is n-dimensional
  Higher dimensions than two shouldn't be needed, these just store the data, all
  actual statistical math is done inside of GSL, and these defined types are
  just for long term storage of data sets

  The sizes of the individual chunks of data are initially changeable so it is
  possible to have IDs, 8-bit numbers, and timestamps work interchangeably.
  However, especially with IDs, some functions are useless and make no sense
  to use (standard deviation, mean, etc). 

  On the fly changing of sizes of data types is not allowed without wiping all
  information (a possible workaround would be loading all of the data into a
  second sample set, copying everything over, changing the sizes, and copying
  everything back with casting/rounding to the proper size, but all use cases
  so far have a well-defined upper limit on size)

  TODO: perhaps create stat_sample_set_chunk_t and load them individually into
  GSL's running statistics functions to cut down on memory usage (if on an SSD,
  would be faster than swap without ruining the drive)
 */

struct stat_sample_set_t{
private:
	std::vector<uint8_t> size_set;
	std::vector<uint8_t> set;
	//cache
	uint32_t entry_size = 0;
public:
	data_id_t id;
	stat_sample_set_t();
	~stat_sample_set_t();
	void reset(std::vector<uint8_t> entry_size);
	void add(std::vector<std::vector<uint8_t> > datum);
	void truncate_to_size(uint64_t size_);
};

#endif
