./clean.sh
gdb --args ./basictv --print_level 0 --audio true --run_tests true | tee output_gdb
nano -v output_gdb
