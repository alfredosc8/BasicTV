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

// type, id, mod_inc

static std::vector<std::tuple<std::array<uint8_t, 32>, id_t_, uint64_t> > disk_index;

static std::string index_from_disk_pull_type(std::string str){
	size_t end = 
		str.find_last_of("_t")+2;
	if(end == std::string::npos){
		print("couldn't find type in path", P_ERR);
	}
	size_t start = 
		str.substr(0, end).find_first_of(
			SLASH)+1;
	if(start-1 == std::string::npos){
		print("couldn't parse type in path", P_ERR);
	}
	const std::string retval =
		str.substr(start, end-start);
	P_V_S(retval, P_SPAM);
	return retval;
}

static id_t_ index_from_disk_pull_id(std::string str){
	id_t_ retval = ID_BLANK_ID;
	retval =
		convert::array::id::from_hex(
			str.substr(
				str.find_last_of(SLASH),
				str.find_last_of('_'))); // should work (?)
	P_V_S(convert::array::id::to_hex(retval), P_SPAM);
	return retval;
}

static uint64_t index_from_disk_pull_mod_inc(std::string str){
	uint32_t retval = 0;
	retval = std::stoi(
		str.substr(
			str.find_first_of('_'),
			str.size()));
	P_V(retval, P_SPAM);
	return retval;
}

void id_api::disk::build_index_from_disk(){
	disk_index.clear();
	std::vector<std::string> raw_index =
		system_handler::find_all_files(
			file::ensure_slash_at_end(
				settings::get_setting(
					"data_folder")));
	for(uint64_t i = 0;i < raw_index.size();i++){
		try{
			const std::string type = 
				index_from_disk_pull_type(raw_index[i]);
			const id_t_ id =
				index_from_disk_pull_id(raw_index[i]);
			const uint64_t mod_inc =
				index_from_disk_pull_mod_inc(raw_index[i]);
			disk_index.push_back(
				std::make_tuple(
					convert::array::type::to(type),
					id,
					mod_inc));
		}catch(...){
			print("caught an exception in disk index builder",
			      P_NOTE);
		}
	}
	print("successfully loaded " + std::to_string(disk_index.size()) +
	      " out of " + std::to_string(raw_index.size()) + " indicies", P_DEBUG);
}

/*
  TODO: actually convert stuff to use the disk functions and destroy the disk
  loading functions in import (and probably all of import too). I feel it is
  too broad, and that a seperate section for requests should be created for
  any networking concerns (gives source code room for growth and optimization)

  It might make sense to have a third bank where just the strings sit in memory
  as a low overhead cache system, but this works fine for now (simple too)
 */

void id_api::disk::load_from_disk(id_t_ id){
	for(uint64_t i = 0;i < disk_index.size();i++){
		if(unlikely(std::get<1>(disk_index[i]) == id)){
			const std::string path_to_file =
				file::ensure_slash_at_end(
					settings::get_setting(
						"data_folder")) +
				convert::array::type::from(std::get<0>(disk_index[i])) +
				std::string(1, SLASH) +
				convert::array::id::to_hex(id) +
				"_" +
				std::to_string(std::get<2>(disk_index[i]));
			P_V_S(path_to_file, P_SPAM);
			// TODO: make a file:: function that's an optomized
			// version of this
			std::ifstream in(path_to_file, std::ios::binary);
			if(in.is_open() == false){
				print("unable to open ID file", P_ERR);
			}
			std::vector<uint8_t> id_data;
			char tmp;
			while(in.get(tmp)){
				id_data.push_back(tmp);
			}
			in.close();
			id_api::array::add_data(id_data);
			
		}
	}
	print("id does not exist on disk per latest disk index", P_SPAM);
}

void id_api::disk::load_from_disk(std::vector<id_t_> ids){
	for(uint64_t i = 0;i < ids.size();i++){
		load_from_disk(ids[i]);
	}
}

void id_api::disk::save_to_disk(id_t_ id){
	//print("move ID exporting code from id_api to id_disk", P_ERR);
	data_id_t *ptr =
		PTR_ID(id, );
	std::vector<uint8_t> exportable_data =
		ptr->export_data(ID_DATA_NONET);
	if(exportable_data.size() == 0){
		print("not going to export blank data", P_DEBUG);
		return;
	}
	const std::string filename =
		get_filename(id);
	system_handler::rm(filename);
	system_handler::mkdir(file::ensure_slash_at_end(settings::get_setting("data_folder"))+ptr->get_type());
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	if(out.is_open() == false){
		print("cannot open file for exporting", P_ERR);
	}
	out.write((const char*)exportable_data.data(), exportable_data.size());
	out.close();

}

void id_api::disk::save_to_disk(std::vector<id_t_> id_vector){
	for(uint64_t i = 0;i < id_vector.size();i++){
		save_to_disk(id_vector[i]);
	}
}

std::string id_api::disk::get_filename(id_t_ id_){
	std::string retval;
	data_id_t *id = PTR_ID(id_, );
	if(id == nullptr){
		return "";
	}
	retval += file::ensure_slash_at_end(settings::get_setting("data_folder"));
	retval += id->get_type() + "/";
	retval += convert::array::id::to_hex(id_) + "_" + std::to_string(id->get_mod_inc()); // + _ + id incrementor (if it existed)
	P_V_S(retval, P_SPAM);
	return retval;
}
