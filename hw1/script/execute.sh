#!/bin/bash
# Put your command below to execute your program.
# Replace "./my-program" with the command that can execute your program.
# Remember to preserve " $@" at the end, which will be the program options we give you.
# ./retriever $@
./retriever -r -i queries/query-train.xml -o ans.csv -m model -d CIRB010
