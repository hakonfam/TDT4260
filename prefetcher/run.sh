#!/bin/bash

prefetcher_file="$1"
until [ -e "$prefetcher_file" ]; do
    read -p "Enter prefetcher file name> " prefetcher_file
done;
cp * ../../framework/prefetcher/
i="../../framework/prefetcher/prefetcher.cc" #Algkons style
cp "$prefetcher_file" "$i"
cd ../../framework
./compile.sh && ./test_prefetcher.py
#build_successful=./compile.sh "$prefetcher_file"
#if [ $build_successful ]; then
#    cd ../../framework
#    ./test_prefetcher.py
#    cd -
#fi;
