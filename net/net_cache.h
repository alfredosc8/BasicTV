#ifndef NET_CACHE_H
#define NET_CACHE_H
/*
  Leftover code from another project, but is a cache for frequently
  requested URLs
 */


struct net_cache_t{
private:
	std::string url;
	std::string data;
	uint64_t timestamp;
	bool complete;
public:
	net_cache_t(std::string url_, std::string data_, uint64_t timestamp_);
	~net_cache_t();
	std::string get_url();
	std::string get_data();
	void set_data(std::string);
	uint64_t get_timestamp();
	void set_complete(bool);
	bool get_complete();
};

#endif
