#!bin/bash


cd build
rm report.callgrind
cmake .. && make && valgrind --tool=callgrind --dump-instr=yes --collect-bus=yes --callgrind-out-file=report.callgrind ./myapp profile 
kcachegrind report.callgrind


exit 0