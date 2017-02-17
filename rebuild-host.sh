#!/bin/sh

# On ARM hosts like Raspberry Pi3, we will likely run out of memory if attempting more than 1 compilation thread:
ncpu=`cat /proc/cpuinfo |grep processor|wc -l`
if [ `cat /proc/cpuinfo | grep ARM | wc -l` -gt 0 ]; then ncpu=1; fi
   
# FIXME: the chmod 777 below is needed on my host to allow root to write in the build dir
/bin/rm -rf hbuild && mkdir hbuild && cd hbuild && cmake "$@" .. && make -j ${ncpu} && chmod 777 . && sudo make install
