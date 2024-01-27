#!/usr/bin/env bash



# cd pa4_student
rm -rf received/
mkdir -p received/
# function to clean up files and make executables
remake () {
    #echo -e "\nCleaning old files and making executables"
    make -s >/dev/null 2>&1
}
echo -e "To remove colour from tests, set COLOUR to 1 in sh file\n"
COLOUR=0
if [[ COLOUR -eq 0 ]]; then
    ORANGE='\033[0;33m'
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    NC='\033[0m'
else
    ORANGE='\033[0m'
    GREEN='\033[0m'
    RED='\033[0m'
    NC='\033[0m'
fi


remake
#echo -e "\nTest cases for datapoint transfers"

IP_ADDR='35.224.171.183'
PORT=10110
echo -e "\nTesting :: ./client -a ${IP_ADDR} -r ${PORT} -n ${N} -p ${P} -w 100 -h 20 -b 5 \n"
N=1000
P=5
./client -a ${IP_ADDR} -r ${PORT} -n ${N} -p ${P} -w 100 -h 20 -b 5 >out.tst
if [ $(comm -12 <(sort out.tst) <(sort test-files/data1.txt) | wc -l) -eq $(cat test-files/data1.txt | wc -l) ]; then
    echo -e "  ${GREEN}Passed${NC}"
else
    echo -e "  ${RED}Failed${NC}"
fi
kill $! 2>/dev/null
wait $! 2>/dev/null



N=10000
P=10
let PORT+=1
echo -e "port num ${PORT}"
echo -e "\nTesting :: ./client -a ${IP_ADDR} -r ${PORT}  -n ${N} -p ${P} -w 100 -h 20 -b 30\n"
sleep 1
./client -a ${IP_ADDR} -r ${PORT}  -n ${N} -p ${P} -w 100 -h 20 -b 30 >out.tst
if [ $(comm -12 <(sort out.tst) <(sort test-files/data2.txt) | wc -l) -eq $(cat test-files/data2.txt | wc -l) ]; then
    echo -e "  ${GREEN}Passed${NC}"
else
    echo -e "  ${RED}Failed${NC}"
fi
kill $! 2>/dev/null
wait $! 2>/dev/null


remake
#echo -e "\nTest cases for csv file transfers"
let PORT+=1
echo -e "port num ${PORT}"
echo -e "\nTesting :: ./client -a ${IP_ADDR} -r ${PORT}  -w 100 -b 30 -f 3.csv ; diff -sqwB BIMDC/3.csv received/3.csv\n"


sleep 1
./client -a ${IP_ADDR} -r ${PORT}  -w 100 -b 30 -f 3.csv >/dev/null
if test -f "received/3.csv"; then
    if diff BIMDC/3.csv received/3.csv >/dev/null; then
        echo -e "  ${GREEN}Passed${NC}"
    else
        echo -e "  ${RED}Failed${NC}"
    fi
else
    echo -e "  ${ORANGE}No 3.csv in received/ directory${NC}"
fi
kill $! 2>/dev/null
wait $! 2>/dev/null


remake
#echo -e "\nTest cases for binary file transfers"

let PORT+=1
echo -e "port num ${PORT}"
echo -e "\nTesting :: truncate -s 256K BIMDC/test.bin; ./client -a ${IP_ADDR} -r ${PORT}  -w 100 -b 50 -f test.bin; diff -sqwB BIMDC/test.bin received/test.bin\n"
truncate -s 256K BIMDC/test.bin


sleep 1
./client -a ${IP_ADDR} -r ${PORT}  -w 100 -b 50 -f test.bin >/dev/null
if test -f "received/test.bin"; then
    if diff BIMDC/test.bin received/test.bin >/dev/null; then
        echo -e "  ${GREEN}Passed${NC}"
    else
        echo -e "  ${RED}Failed${NC}"
    fi
else
    echo -e "  ${ORANGE}No test.bin in received/ directory${NC}"
fi
kill $! 2>/dev/null
wait $! 2>/dev/null


truncate -s 256K BIMDC/test.bin

let PORT+=1
echo -e "port num ${PORT}"
echo -e "\nTesting :: truncate -s 256K BIMDC/test.bin; ./client -a ${IP_ADDR} -r ${PORT}  -w 100 -b 50 -m 4096 -f test.bin; diff -sqwB BIMDC/test.bin received/test.bin\n"
sleep 1
./client -a ${IP_ADDR} -r ${PORT}  -w 100 -b 50 -m 4096 -f test.bin >/dev/null
if test -f "received/test.bin"; then
    if diff BIMDC/test.bin received/test.bin >/dev/null; then
        echo -e "  ${GREEN}Passed${NC}"
    else
        echo -e "  ${RED}Failed${NC}"
    fi
else
    echo -e "  ${ORANGE}No test.bin in received/ directory${NC}"
fi
kill $! 2>/dev/null
wait $! 2>/dev/null

echo -e "\n"
exit 0
