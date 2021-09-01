#!/bin/sh
# USAGE: rebuild-pro-platform.sh [cmake opts]

# compiled code is in /var/lib/jevoispro-build/ and /var/lib/jevoispro-microsd/

# On JeVoisPro, limit the number of compile threads to not run out of memory:
ncpu=`grep -c processor /proc/cpuinfo`
if [ `grep -c JeVois /proc/cpuinfo` -gt 0 ]; then ncpu=4; fi

sudo /bin/rm -rf ppbuild \
    && mkdir ppbuild \
    && cd ppbuild \
    && cmake "$@" -DJEVOIS_HARDWARE=PRO -DJEVOIS_PLATFORM=ON .. \
    && make -j ${ncpu} \
    && sudo make install
