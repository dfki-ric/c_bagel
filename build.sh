#! /bin/bash

echo  -e "\033[32;1m"
echo "********** build c_bagel library **********"
echo -e "\033[0m"

rm -rf build
mkdir build
cd build
cmake_debug
make -j4
cd ..

echo  -e "\033[32;1m"
echo "********** done building c_bagel library **********"
echo -e "\033[0m"
