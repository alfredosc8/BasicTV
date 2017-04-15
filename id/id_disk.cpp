#include "id_disk.h"
#include "id_api.h"
#include "id.h"
#include "../settings.h"
#include "../file.h"
#include "../system.h"
#include "../convert.h"
#include "../util.h"
#include "../main.h"

/*
  This is a rewritten and improved version of import with some better
  internal workings
 */

// medium data, type, id, mod_inc

static std::vector<std::tuple<type_t_, id_t_, uint64_t> > disk_index;

// static std::string index_from_disk_pull_type(std::string str){
// 	size_t end = 
// 		str.find_last_of("_t")+2;
// 	if(end == std::string::npos){
// 		print("couldn't find type in path", P_ERR);
// 	}
// 	size_t start = 
// 		str.substr(0, end).find_first_of(
// 			SLASH)+1;
// 	if(start-1 == std::string::npos){
// 		print("couldn't parse type in path", P_ERR);
// 	}
// 	const std::string retval =
// 		str.substr(start, end-start);
// 	P_V_S(retval, P_SPAM);
// 	return retval;
// }

// static id_t_ index_from_disk_pull_id(std::string str){
// 	id_t_ retval = ID_BLANK_ID;
// 	retval =
// 		convert::array::id::from_hex(
// 			str.substr(
// 				str.find_last_of(SLASH),
// 				str.find_last_of('_'))); // should work (?)
// 	P_V_S(convert::array::id::to_hex(retval), P_SPAM);
// 	return retval;
// }

// static uint64_t index_from_disk_pull_mod_inc(std::string str){
// 	uint32_t retval = 0;
// 	retval = std::stoi(
// 		str.substr(
// 			str.find_first_of('_'),
// 			str.size()));
// 	P_V(retval, P_SPAM);
// 	return retval;
// }

// void id_api::disk::build_index_from_disk(){
// 	disk_index.clear();
// 	std::vector<std::string> raw_index =
// 		system_handler::find_all_files(
// 			file::ensure_slash_at_end(
// 				settings::get_setting(
// 					"data_folder")));
// 	for(uint64_t i = 0;i < raw_index.size();i++){
// 		try{
// 			const std::string type = 
// 				index_from_disk_pull_type(raw_index[i]);
// 			const id_t_ id =
// 				index_from_disk_pull_id(raw_index[i]);
// 			const uint64_t mod_inc =
// 				index_from_disk_pull_mod_inc(raw_index[i]);
// 			disk_index.push_back(
// 				std::make_tuple(
// 					convert::type::to(type),
// 					id,
// 					mod_inc));
// 		}catch(...){
// 			print("caught an exception in disk index builder",
// 			      P_NOTE);
// 		}
// 	}
// 	print("successfully loaded " + std::to_string(disk_index.size()) +
// 	      " out of " + std::to_string(raw_index.size()) + " indicies", P_DEBUG);
// }

/*
  TODO: instead of doing a simple search on everything, develop preferences
  towards certain types of data on certain drives, and search for those
  first (assuming I decide to add type data to the ID in the near future).

  Or, at the very least, sort it by the raw size of the ID as a number, so we
  have some basic efficiency going on here.
 */

static id_t_ optimal_disk_id_load(id_t_ id){
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
				return disk_id_vector[i];
			}
		}
	}
	return ID_BLANK_ID;
}

// for(uint64_t i = 0;i < disk_index.size();i++){
// 	if(unlikely(std::get<1>(disk_index[i]) == id)){
// 		const std::string path_to_file =
// 			file::ensure_slash_at_end(
// 				data_folder_from_disk_id(
// 					disk_id)) +
// 			convert::type::from(std::get<0>(disk_index[i])) +
// 			std::string(1, SLASH) +
// 			convert::array::id::to_hex(id) +
// 			"_" +
// 			std::to_string(std::get<2>(disk_index[i]));
// 		P_V_S(path_to_file, P_SPAM);
// 		// TODO: make a file:: function that's an optomized
// 		// version of this
// 		std::ifstream in(path_to_file, std::ios::binary);
// 		if(in.is_open() == false){
// 			print("unable to open ID file", P_ERR);
// 		}
// 		std::vector<uint8_t> id_data;
// 		char tmp;
// 		while(in.get(tmp)){
// 			id_data.push_back(tmp);
// 		}
// 		in.close();
// 		id_api::array::add_data(id_data);
			
// 	}
// }


static id_t_ optimal_disk_id_save(id_t_ id_){
	/*
	  When I get numbers like access times, linked list lengths, and
	  hard drive/ssd numbers like latency and speed, I would love to dive
	  into this a lot more, but since I don't, this is just going to
	  search for (probably the only) available drive that has the most
	  free space
	 */
	// or I can just randomize it
	std::vector<id_t_> disk_id_vector =
		id_api::cache::get(
			TYPE_ID_DISK_INDEX_T);
	if(disk_id_vector.size() >= 1){
		return disk_id_vector[0];
	}
	print("no id_disk_index_t exists", P_CRIT);
	return ID_BLANK_ID;
}

void id_disk_api::load(id_t_ id){
	id_t_ disk_id =
		optimal_disk_id_load(id);
	id_disk_index_t *disk_index_ptr =
		PTR_DATA(disk_id,
			 id_disk_index_t);
	if(disk_index_ptr == nullptr){
		print("cannot find a valid disk ID with requested ID information", P_NOTE);
		return;
		// should probably be P_SPAM
	}
	// loads it directly into memory, no return value
	disk_index_ptr->import_id(id);
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
		P_V_S(id_hex, P_SPAM); // checking my work, not important
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
		PTR_ID(id_, );
	if(id == nullptr){
		// filename won't be completel without mod_inc (technically)
		return "";
	}
	retval += file::ensure_slash_at_end((char*)(path.data()));
	retval += id->get_type() + "/";
	retval += convert::array::id::to_hex(id_) + "_" + std::to_string(id->get_mod_inc()); // + _ + id incrementor (if it existed)
	P_V_S(retval, P_SPAM);
	return retval;
}

void id_disk_index_t::export_id(id_t_ id_){
	data_id_t *ptr =
		PTR_ID(id_, );
	std::vector<uint8_t> exportable_data =
		ptr->export_data(ID_DATA_NONET,
				 ID_EXTRA_COMPRESS | ID_EXTRA_ENCRYPT);
	std::string filename =
		get_path_of_id(id_);
	system_handler::rm(filename);
	system_handler::mkdir(
		file::ensure_slash_at_end(
			(char*)(path.data()))
		+ptr->get_type());
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	if(out.is_open() == false){
	 	print("cannot open file for exporting", P_ERR);
	}
	out.write((const char*)exportable_data.data(), exportable_data.size());
	out.close();
}

void id_disk_index_t::import_id(id_t_ id_){
	print("actually import data", P_CRIT);
}

bool id_disk_index_t::id_on_disk(id_t_ id_){
	for(uint64_t i = 0;i < index.size();i++){
		if(index[i] == id_){
			return true;
		}
	}
	return false;
}
