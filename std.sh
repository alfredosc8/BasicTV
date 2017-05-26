./clean.sh
#valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all 
gdb --args ./basictv --print_level 0 --print_stats true --print_delay 100 --print_backtrace true
