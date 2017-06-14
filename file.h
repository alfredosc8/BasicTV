#ifndef FILE_H
#define FILE_H
#include <fstream>
#include <exception>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __linux
#define SLASH '/'
#else
#error finish writing this macro
#endif
namespace file{
	void write_file(std::string file, std::string data);
	std::string read_file(std::string file);
	bool exists(std::string file);
	void write(std::string file, std::string data);
	std::string read(std::string file);
	void wait_for_file(std::string file);
	bool is_dir(std::string path);
	bool is_file(std::string path);
	std::string ensure_slash_at_end(std::string str);

	std::vector<uint8_t> read_file_vector(std::string file);
	void write_file_vector(std::string file, std::vector<uint8_t> raw_data);
}
#endif
