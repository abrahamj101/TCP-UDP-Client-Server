#!/usr/bin/env bash

# function to clean up files and make executables
remake () {
    #echo -e "\nCleaning old files and making executables"
    make -s >/dev/null 2>&1
}
cd pa4_student
remake


# Start port number
start_port=10110

# Number of servers to start
num_servers=4

for (( i=0; i<num_servers; i++ ))
do
    port=$((start_port + i))
    ./server -r ${port} >/dev/null &
    echo "Server started on port ${port}"
done

rm -rf BIMDC/test.bin
truncate -s 256K BIMDC/test.bin
port=$((start_port + i))
./server -r ${port} -m 4096 >/dev/null &
echo "Server started on port ${port}"
echo "All servers started."