#!/bin/sh

# On ARM hosts like Raspberry Pi3, we will likely run out of memory if attempting more than 1 compilation thread:
ncpu=`cat /proc/cpuinfo |grep processor|wc -l`
if [ `cat /proc/cpuinfo | grep ARM | wc -l` -gt 0 ]; then ncpu=1; fi

/bin/rm -rf pbuild && mkdir pbuild && cd pbuild && cmake "$@" -DJEVOIS_PLATFORM=ON .. && make -j ${ncpu} && make install
