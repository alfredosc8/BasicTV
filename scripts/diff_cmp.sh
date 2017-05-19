for i in {1..1000}
do
    for c in {1..1000}
    do
	if [[ i -eq c ]]
	then
	    continue
	fi
	# IDs are always different, remove all references
	echo Diff between $i and $c
	diff valgrind_basictv_out_$i valgrind_basictv_out_$c | grep -v "convert::array::id" | grep -v "id_" | grep -v "_id"
	echo Press [ENTER] to continue
	read
    done
done

