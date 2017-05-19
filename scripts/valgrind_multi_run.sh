./valgrind_clean.sh
for i in {1..1000}
do
    echo Running $i
    #valgrind --leak-check=full --track-origins=yes
    # might want to remove init_close_only later on, but it works fine for what i'm doing
    ../basictv --init_close_only true --data_folder VALGRIND_DATA_FOLDER_$i | tee valgrind_basictv_out_$i &
done
