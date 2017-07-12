#ifndef ID_DISK_H
#define ID_DISK_H

#include "string"
#include "../util.h"
#include "../main.h"

/*
  I'm rewriting all of the ID loading/management system inside
  id_tier.*, so this file is here to gracefully convert
 */

/*
  This is bound to a certain directory (or other means of storage), and is
  responsible for keeping track of the state of all IDs inside of that
  folder. This should work well, assuming rebuilds happen on restarts and
  there is no overlapping (or overlap protection) between locations.
 */

/*
  How is the information reaching there (most users will use DIR, directory/
  direct). 
 */

#define ID_DISK_TRANS_UNDEF 0
#define ID_DISK_TRANS_DIR 1
#define ID_DISK_TRANS_SSH 2 // SCP
#define ID_DISK_TRANS_FTP 3

/*
  Where is the information stored (to not kill SSDs, no long term storage for
  often used data, internal moving from one another to save fast access mediums
  for often used data, etc.)
 */

#define ID_DISK_MEDIUM_UNDEF 0
#define ID_DISK_MEDIUM_HDD 1
#define ID_DISK_MEDIUM_SSD 2
#define ID_DISK_MEDIUM_DISK_LIBRARY 3
#define ID_DISK_MEDIUM_TAPE_LIBRARY 4
// not implemented, but would be pretty cool
#define ID_DISK_MEDIUM_IPFS 5

/*
  Certain enhancements (pushed back to a byte vector), used in conjunction with
  the other flags to more accurately reflect real world performance

  MULTIDISK_READ and MULTIDISK_WRITE are going to be used to denote RAID
  configurations as well as other RAID-like configurations, based on the
  effective speed increase
  RAID 0: READ and WRITE
  RAID 1: READ
  RAID 5: READ
  RAID 10: READ and WRITE
  BTRFS multi-disk (balanced): READ and WRITE

  It would be better if the id_disk_index_t, instead of exporting individual
  pieces of data, would instead create "large" files (anywhere from 512Kb+)
  that contain a list of IDs, for the following reasons:
  1. Plaintext information (or non-compressed information) that I own can be
  compressed inter-file
  2. We can get around the pesky 4K block size

  Of course, this idea would have to be done on a case by case basis, because
  sending more information to this computer over the network (in the case
  of all current non-direct mediums) isn't effective (unless like IDs are
  exported to disk, but the odds of using 100% of the data 100% of the time
  is zero).
 */

#define ID_DISK_ENHANCE_UNDEF 0
#define ID_DISK_ENHANCE_MULTIDISK_READ 1
#define ID_DISK_ENHANCE_MULTIDISK_WRITE 2
#define ID_DISK_ENHANCE_READ_ONLY 3

#define ID_DISK_PATH_LENGTH 256

/*
  TODO: Get some stats from disk reads/writes and incorporate that as well
 */

struct id_disk_index_t{
private:
	uint8_t medium = 0;
	uint8_t tier = 0;
	uint8_t transport = 0;
	std::vector<uint8_t> enhance;
	std::array<uint8_t, ID_DISK_PATH_LENGTH> path = {{0}};
	/*
	  TODO:
	  1. Sort the index
	  2. Allow storing multiple IDs in one file to get around pesky limits
	  3. (Possibly) bring back ID_DISK_ENHANCE_COMPRESSION for data that I
	  create, and archive the exported IDs as large unencrypted and
	  uncompressed files, but instead of the OS/FS handling that, we
	  compress it ourselves (since a majority of users don't run BTRFS or
	  another comparable FS).
	*/
	std::vector<id_t_> index;
	uint64_t kb_used = 0;
	uint64_t kb_total = 0;
	// TODO: create a function that fetches mod_inc (when implemented),
	// as well as other information about the ID that isn't included by
	// ID reference
	void update_index_from_disk();
public:
	data_id_t id;
	id_disk_index_t();
	~id_disk_index_t();
	// getters and setters
	void set(uint8_t medium_, uint8_t tier_, uint8_t transport_, std::vector<uint8_t> enhance_, std::string path_);
	
	std::string get_path(){return (char*)(path.data());}
	uint8_t get_medium(){return medium;}
	uint8_t get_tier(){return tier;}
	uint8_t get_transport(){return transport;}
	std::vector<id_t_> get_index(){return index;}
	bool has_enhance(uint8_t enhance_);
	uint64_t get_kb_used(){return kb_used;}
	uint64_t get_kb_total(){return kb_total;}
	
	std::string get_path_of_id(id_t_ id_);
	bool id_on_disk(id_t_ id_);
	
	// transporting data to and from disk
	void export_id(id_t_ id_);
	void import_id(id_t_ id_);
};

namespace id_disk_api{
	/*
	  All disks should be abstracted out, lookups and queries into
	  larger tables should be fine for now and forever
	*/
	void save(std::vector<id_t_>);
	void save(id_t_);
	void load(std::vector<id_t_>);
	void load(id_t_);
	std::string get_filename(id_t_);
};

#endif
