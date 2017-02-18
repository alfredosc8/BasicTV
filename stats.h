#ifndef STATS_H
#define STATS_H
#include <vector>
#include <tuple>
#include "id/id.h"
/*
  stats.h a simple statistics library
 */

/*
  TODO: expand this to get some general stats information. I need to pass a 
  vector of the data, and doing statistics on the IDs themselves is useless
  (perhaps specify an data_vector entry?)
 */

// virtual class, standard doesn't mean a standard curve
struct stat_sample_set_standard_t{
protected:
	uint64_t max_samples = 0;
public:
	stat_sample_set_standard_t();
	~stat_sample_set_standard_t();
	void list_virtual_data(data_id_t *id);
	void set_max_samples(uint64_t max_samples_);
	uint64_t get_max_samples();
};

/*
  stat_sample_set_t: a data set that plots two 64-bit integers.
  Currently only used for bandwidth monitoring, but should be the only needed
  data set (non-ID)
 */

struct stat_sample_set_t : virtual stat_sample_set_standard_t{
private:
	std::vector<uint64_t> x;
	std::vector<uint64_t> y;
public:
	data_id_t id;
	stat_sample_set_t();
	~stat_sample_set_t();
	void add_sample(uint64_t x, uint64_t y);
	std::vector<uint64_t> get_x();
	std::vector<uint64_t> get_y();
	void set_tables(std::vector<uint64_t>, std::vector<uint64_t>);
};

/*
  stat_sample_set_id_t: a data set that plots a 64-bit and IDs
  Currently only used by the net_proto_socket_t struct to get statistical odds
  that a peer has wanted information.
*/

struct stat_sample_set_id_t : virtual stat_sample_set_standard_t{
private:
	/* std::vector<std::pair<uint64_t, id_t_> > sample_vector; */
	std::vector<uint64_t> x;
	std::vector<id_t_> y;
public:
	data_id_t id;
	stat_sample_set_id_t();
	~stat_sample_set_id_t();
	void add_sample(uint64_t x, id_t_ y);
	std::vector<uint64_t> get_x();
	std::vector<id_t_> get_y();
	void set_tables(std::vector<uint64_t>, std::vector<id_t_>);
};

#endif
