#include "console.h"

static std::string gen_table(std::vector<std::vector<std::string> > entries){
	std::string retval;
	std::vector<uint16_t> row_length;
	for(uint64_t x = 0;x < entries.size();x++){
		for(uint64_t y = 0;y < entries[x].size();y++){
			while(row_length.size() <= y){
				row_length.push_back(0);
			}
			if(entries[x][y].size() > row_length[y]){
				row_length[y] = entries[x][y].size();
			}
		}
	}
	for(uint64_t x = 0;x < entries.size();x++){
		std::string row;
		for(uint64_t y = 0;y < entries[x].size();y++){
			row += " | " + fix_to_length(entries[x][y], row_length[y]);
		}
		row += " | ";
		retval += row + "\n";
	}
	P_V_S(retval, P_DEBUG);
	return retval + "\n";
}

DEC_CMD(print_output_table){
	print_socket(
		gen_table(
			output_table));
}

DEC_CMD(print_reg){
	print_socket(
		gen_table(
		{std::vector<std::string>(
				registers.begin(),
				registers.end())}));
}
