make -j7
rm tmp_output
./basictv --run_tests true | tee tmp_output
nano tmp_output
