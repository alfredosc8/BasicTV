valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./basictv --print_level 0 | tee output_valgrind
