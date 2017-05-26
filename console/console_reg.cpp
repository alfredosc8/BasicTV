#include "console.h"

DEC_CMD_VECTOR(reg_set_const){
	const uint8_t entry = std::stoi(cmd_vector.at(0));
	registers.at(entry) = cmd_vector.at(1);
}

DEC_CMD_VECTOR(reg_set_table){
	const uint8_t entry = std::stoi(cmd_vector.at(0));
	const uint64_t x = std::stoi(cmd_vector.at(1));
	const uint64_t y = std::stoi(cmd_vector.at(2));
	registers.at(entry) = output_table.at(x).at(y);
}

DEC_CMD_VECTOR(reg_copy){
	const uint8_t first_entry = std::stoi(cmd_vector.at(0));
	const uint8_t second_entry = std::stoi(cmd_vector.at(1));
	registers.at(first_entry) = registers.at(second_entry);
}

DEC_CMD_VECTOR(reg_swap){
	const uint8_t first_entry =
		std::stoi(cmd_vector.at(0));
	const uint8_t second_entry =
		std::stoi(cmd_vector.at(1));
	const std::string tmp =
		registers.at(first_entry);
	registers.at(first_entry) = registers.at(second_entry);
	registers.at(second_entry) = tmp;
	
}

DEC_CMD_VECTOR(reg_clear){
	const uint8_t entry = std::stoi(cmd_vector.at(0));
	registers.at(entry) = "";
}

DEC_CMD_VECTOR(reg_left_shift){
	const uint8_t magnitude = std::stoi(cmd_vector.at(0));
	std::array<std::string, CONSOLE_REGISTER_SIZE> old = registers;
	for(uint64_t i = CONSOLE_REGISTER_SIZE-magnitude-1;i < CONSOLE_REGISTER_SIZE;i++){
		registers.at(i) = "";
	}
	for(uint64_t i = magnitude;i < CONSOLE_REGISTER_SIZE;i++){
		registers.at(i-magnitude) = old.at(i);
	}
}

DEC_CMD_VECTOR(reg_right_shift){
	const uint8_t magnitude = std::stoi(cmd_vector.at(0));
	std::array<std::string, CONSOLE_REGISTER_SIZE> old = registers;
	for(uint64_t i = 0;i < magnitude;i++){
		registers.at(i) = "";
	}
	for(uint64_t i = 0;i < (uint64_t)(CONSOLE_REGISTER_SIZE)-magnitude;i++){
		registers.at(i+magnitude) = old.at(i);
	}
}
