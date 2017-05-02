./reset_all.sh
../basictv --init_close_only true
echo "Sleeping to check closing condition of first run"
sleep 5
../basictv
