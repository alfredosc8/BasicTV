#include "system.h"
#include "file.h"
#include "util.h"

static std::string gen_unique_filename(std::string filename){
	// TODO: get a standard way of determining max
	// file lengths (65536 is a soft limit)
	return filename + std::to_string(true_rand(0, 65536));
}

int system_handler::run(std::string str){
	str += ";touch finished 2>&1 /dev/null";
	/*
	  Most commands need some time to be processed on the lower level (GPIO).
	  Speed shouldn't be a problem
	*/
	int retval = system(str.c_str());
	std::string filename =
		gen_unique_filename("finished");
	file::wait_for_file(filename);
	rm(filename);
	return retval;
}

void system_handler::write(std::string cmd, std::string file){
	run(cmd + " | tee " + file);
	file::wait_for_file(file);
}

void system_handler::mkdir(std::string dir){
	run("mkdir -p " + dir);
}

std::string system_handler::cmd_output(std::string cmd){
	std::string filename =
		gen_unique_filename("TMP_OUT");
	write(cmd, filename);
	const std::string file_data = file::read_file(filename);
	rm(filename);
	return file_data;
}

void system_handler::rm(std::string file){
	// TODO: since i'm piping the output, should escape quotes in file
	system(("rm -r '" + file + "' 2> /dev/null 1> /dev/null").c_str());
}

/*
  TODO: when I implement this for Windows, get a general purpose search function
  for std::vector<std::string> that functions like grep
 */

std::vector<std::string> system_handler::find(std::string directory, std::string search){
	// no need for anything more advanced right now
	std::vector<std::string> retval;
	if(search != ""){
		retval =
			newline_to_vector(
				cmd_output("find " + directory + " | grep " + search));;
	}else{
		retval =
			newline_to_vector(
				cmd_output("find " + directory));
	}
	if(retval.size() > 1){
		retval.erase(retval.begin()); // directory itself
	}
	return retval;
}

std::vector<std::string> system_handler::find_all_files(std::string directory, std::string search){
	std::vector<std::string> retval =
		system_handler::find(directory, search);
	for(uint64_t i = 0;i < retval.size();i++){
		if(!file::is_file(retval[i])){
			retval.erase(
				retval.begin()+i);
			i--;
		}
	}
	return retval;
}
