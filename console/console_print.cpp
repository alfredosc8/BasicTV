#include "console.h"

static std::string gen_table(std::vector<std::vector<std::string> > entries){
	std::string retval;
	std::vector<uint16_t> row_length;
	for(uint64_t y = 0;y < entries.size();y++){
		for(uint64_t x = 0;x < entries[y].size();x++){
			while(row_length.size() <= x){
				row_length.push_back(0);
			}
			if(entries[y][x].size() > row_length[x]){
				row_length[x] = entries[y][x].size();
			}
		}
	}
	int64_t end = 0;
	for(int64_t y = 0;y < (int64_t)entries.size();y++){
		for(int64_t x = 0;x < (int64_t)entries[y].size();x++){
			if(entries[y][x] != "" && x > end){
				end = x;
			}
		}
	}
	P_V(end, P_VAR);
	for(uint64_t y = 0;y < entries.size();y++){
		std::string row;
		for(uint64_t x = 0;x <= (uint64_t)end;x++){
			row += " | " + fix_to_length(entries[y][x], row_length[x]);
		}
		row += " | ";
		retval += row + "\n";
	}
	P_V_S(retval, P_VAR);
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

DEC_CMD(print_reg_with_type){
	std::vector<std::string> parody;
	for(uint64_t i = 0;i < registers.size();i++){
		std::string data = "";
		try{
			id_t_ tmp_id =
				convert::array::id::from_hex(
					registers[i]);
			data_id_t *id_ptr =
				PTR_ID(tmp_id, );
			if(id_ptr != nullptr){
				data += id_ptr->get_type();
			}
		}catch(...){}
		parody.push_back(data);
	}
	print_socket(
		gen_table(
		{std::vector<std::string>(
				registers.begin(),
				registers.end()),
		 parody}));
}
