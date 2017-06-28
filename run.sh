./clean.sh
#disabling print color makes debugging on the fly pretty hard, but cleans out output_gdb
gdb --args ./basictv --print_level 2 --print_color false | tee output_gdb
nano -v output_gdb
