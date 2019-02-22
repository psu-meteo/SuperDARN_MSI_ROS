cmake_minimum_required (VERSION 2.6)
project (MakeTSG-test)
include_directories("../include")
add_executable(MakeTSG-test test-main.cpp maketsg-test.cpp ../src/maketsg.c catch.hpp ../include/maketsg.h ../include/tsg.h)
