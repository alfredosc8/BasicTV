#include "id_disk.h"
#include "id_api.h"
#include "id.h"
#include "../settings.h"
#include "../file.h"
#include "../system.h"
#include "../convert.h"
#include "../util.h"
#include "../main.h"

#include <iterator>

/*
  This is a rewritten and improved version of import with some better
  internal workings
 */

/*
  TODO: In moving over to the vector of stuff, it would make more sense to
  put the faster mediums first.

  I would also like to get an estimated size of all data types and look
  it up in the table, so we aren't spending a lot of computing time optimizing
  reading the simple data types.
 */

static std::vector<id_t_> optimal_disks_id_load(id_t_ id){
	std::vector<id_t_> retval;
	std::vector<id_t_> disk_id_vector =
		id_api::cache::get(
			"id_disk_index_t");
	for(uint64_t i = 0;i < disk_id_vector.size();i++){
		id_disk_index_t *disk_index_ptr =
			PTR_DATA(disk_id_vector[i],
				 id_disk_index_t);
		if(disk_index_ptr == nullptr){
			print("disk_index_ptr is a nullptr", P_WARN);
		}
		std::vector<id_t_> index =
			disk_index_ptr->get_index();
		for(uint64_t i = 0;i < index.size();i++){
			if(unlikely(index[i] == id)){
				retval.push_back(
					disk_id_vector[i]);
			}
		}
	}
	return retval;
}

static id_t_ optimal_disk_id_save(id_t_ id_){
	/*
	  TODO: If I do get an average size of each data type, I would love
	  to optimize for speed of writing and latencies as well (linear
	  regression of size over time should work fine).

	  I am also quite interested in weighing disk performance, disk space,
	  RAM speed, RAM size, network performance, and CPU performance together
	  to (possibly) have better judgement on placement and execution of
	  things.
	 */
	std::vector<id_t_> disk_id_vector =
		id_api::cache::get(
			TYPE_ID_DISK_INDEX_T);
	if(disk_id_vector.size() == 1){
		return disk_id_vector[0];
	}else{
		for(uint64_t i = 0;i < disk_id_vector.size();i++){
			id_disk_index_t *disk_index_ptr =
				PTR_DATA(disk_id_vector[i],
					 id_disk_index_t);
			if(disk_index_ptr == nullptr){
				continue;
			}
			if(disk_index_ptr->has_enhance(ID_DISK_ENHANCE_READ_ONLY)){
				print("can't save to a read only medium, skipping", P_SPAM);
			}
			if(disk_index_ptr->id_on_disk(id_)){
				return disk_id_vector[i];
			}
		}
	}
	print("no id_disk_index_t exists", P_CRIT);
	return ID_BLANK_ID;
}

void id_disk_api::load(id_t_ id_){
	// id_t_ disk_id =
	// 	optimal_disk_id_load(id);
	// id_disk_index_t *disk_index_ptr =
	// 	PTR_DATA(disk_id,
	// 		 id_disk_index_t);
	// if(disk_index_ptr == nullptr){
	// 	print("cannot find a valid disk ID with requested ID information", P_NOTE);
	// 	return;
	// 	// should probably be P_SPAM
	// }
	// // loads it directly into memory, no return value
	// disk_index_ptr->import_id(id);

	// That's fine from the looks of it, but is pretty advanced relative to
	// thre rest of the software, so just go through everything to make sure
	// nothing weird is happening
	std::vector<id_t_> disk_indexes_list =
		id_api::cache::get(
			"id_disk_index_t");
	for(uint64_t i = 0;i < disk_indexes_list.size();i++){
		id_disk_index_t *disk_index_ptr =
			PTR_DATA(disk_indexes_list[i],
				 id_disk_index_t);
		if(disk_index_ptr == nullptr){
			continue;
		}
		try{
			disk_index_ptr->import_id(id_);
			print("import_id didn't throw an error, ID is probably loaded", P_SPAM);
			break;
		}catch(...){
			print("couldn't find data on disk", P_SPAM);
		}
	}
}

void id_disk_api::load(std::vector<id_t_> ids){
	for(uint64_t i = 0;i < ids.size();i++){
		load(ids[i]);
	}
}

/*
  TODO: see the header file for laundry list
 */

void id_disk_api::save(id_t_ id){
	if(PTR_ID(id, ) == nullptr){
		print("ID to export doesn't exist in memory, aborting", P_WARN);
		return;
	}
	id_t_ disk_id =
		optimal_disk_id_save(id);
	id_disk_index_t *disk_index_ptr =
		PTR_DATA(disk_id,
			 id_disk_index_t);
	if(disk_index_ptr == nullptr){
		print("disk_index_ptr is a nullptr, cannot export to disk", P_WARN);
		print("TODO: allow for creating lists of optimal disks", P_NOTE);
	}else{
		disk_index_ptr->export_id(id);
	}

}

void id_disk_api::save(std::vector<id_t_> id_vector){
	for(uint64_t i = 0;i < id_vector.size();i++){
		save(id_vector[i]);
	}
}

std::string id_disk_api::get_filename(id_t_ id_){
	data_id_t *id = PTR_ID(id_, );
	if(id == nullptr){
		// only runs for this check
		return "";
	}
	std::vector<id_t_> disk_id_vector =
		id_api::cache::get(
			"id_disk_index_t");
	for(uint64_t i = 0;i < disk_id_vector.size();i++){
		id_disk_index_t *disk_index_ptr =
			PTR_DATA(disk_id_vector[i],
				 id_disk_index_t);
		if(disk_index_ptr == nullptr){
			print("disk_index_ptr is a nullptr", P_WARN);
			continue;
		}
		std::string retval;
		if(disk_index_ptr->id_on_disk(id_) &&
		   (retval = disk_index_ptr->get_path_of_id(id_)) != ""){
			return retval;
		}
	}
	return "";
}

id_disk_index_t::id_disk_index_t() : id(this, TYPE_ID_DISK_INDEX_T){
}

id_disk_index_t::~id_disk_index_t(){
}

void id_disk_index_t::update_index_from_disk(){
	std::vector<std::string> find_output =
		system_handler::find_all_files(
			(char*)(path.data()),
			"_t");
	uint64_t old_index_size = index.size();
	index.clear();
	for(uint64_t i = 0;i < find_output.size();i++){
		std::string id_hex =
			find_output[i].substr(
				find_output[i].find_last_of(SLASH)+1,
			        find_output[i].find_last_of('_')-find_output[i].find_last_of(SLASH)-1);
		P_V_S(id_hex, P_VAR); // checking my work, not important
		// TODO: might want to store types and other info as well...
		index.push_back(
			convert::array::id::from_hex(id_hex));
	}
	print("completed a full index refresh (" + std::to_string(old_index_size) + " to " + std::to_string(index.size()) + ")", P_NOTE);
}

void id_disk_index_t::set(uint8_t medium_, uint8_t tier_, uint8_t transport_, std::vector<uint8_t> enhance_, std::string path_){
	if(path_.size() > ID_DISK_PATH_LENGTH){
		print("path is too large, not setting up", P_ERR);
	}
	system_handler::mkdir(path_);
	memcpy(path.data(), path_.data(), path_.size());
	medium = medium_;
	tier = tier_;
	transport = transport_;
	enhance = enhance_;
	update_index_from_disk();
}

std::string id_disk_index_t::get_path_of_id(id_t_ id_){
	std::string retval;
	data_id_t *id =
		PTR_ID_FAST(id_, );
	if(id == nullptr){
		print("ID doesn't exist in memory already, searching disk for ID", P_NOTE);
		std::vector<std::string> rgrep_output =
			system_handler::find_all_files(
				get_path(),
				convert::array::id::to_hex(id_));
		if(rgrep_output.size() >= 1){
			print("picking first rgrep output", P_NOTE);
			print("TODO: fix mod_inc searching (or replace it)", P_NOTE);
			retval = rgrep_output[0];
		}else{
			return "";
		}
	}else{
		print("ID exists in memory already, generating new filename", P_SPAM);
		retval += file::ensure_slash_at_end((char*)(path.data()));
		retval += id->get_type() + "/";
		retval += convert::array::id::to_hex(id_) + "_" + std::to_string(id->get_mod_inc()); // + _ + id incrementor (if it existed)
	}
	P_V_S(retval, P_VAR);
	return retval;
}

void id_disk_index_t::export_id(id_t_ id_){
	data_id_t *ptr =
		PTR_ID(id_, );
	std::vector<uint8_t> exportable_data =
		ptr->export_data(ID_DATA_NONET,
				 ID_EXTRA_COMPRESS | ID_EXTRA_ENCRYPT);
	if(exportable_data.size() == 0){
		print("no data to export from ID", P_SPAM);
		return;
	}
	std::string filename =
		get_path_of_id(id_);
	system_handler::rm(filename);
	system_handler::mkdir(
		file::ensure_slash_at_end(
			get_path())
		+ptr->get_type());
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	if(out.is_open() == false){
	 	print("cannot open file for exporting", P_ERR);
	}
	out.write((const char*)exportable_data.data(), exportable_data.size());
	out.close();
}

void id_disk_index_t::import_id(id_t_ id_){
	std::string filename =
		get_path_of_id(id_);
	std::ifstream in(get_path_of_id(id_), std::ios::in | std::ios::binary);
	if(in.is_open() == false){
		print("couldn't open file for ID", P_ERR);
	}
	in >> std::noskipws;
	std::vector<uint8_t> full_file;
	std::copy(
		std::istream_iterator<uint8_t>(in),
		std::istream_iterator<uint8_t>(),
		std::back_inserter(full_file));
	in.close();
	P_V(full_file.size(), P_VAR);
	id_api::array::add_data(full_file, false);
}

bool id_disk_index_t::id_on_disk(id_t_ id_){
	for(uint64_t i = 0;i < index.size();i++){
		if(unlikely(index[i] == id_)){
			return true;
		}
	}
	return false;
}

bool id_disk_index_t::has_enhance(uint8_t enhance_){
	for(uint64_t i = 0;i < enhance.size();i++){
		if(unlikely(enhance[i] == enhance_)){
			return true;
		}
	}
	return false;
}
