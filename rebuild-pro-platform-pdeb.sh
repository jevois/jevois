#!/bin/sh
# USAGE: rebuild-pro-platform-pdeb.sh [cmake opts]

# compiled code is in /var/lib/jevoispro-build-pdeb/

# On JeVoisPro, limit the number of compile threads to not run out of memory:
ncpu=`grep -c processor /proc/cpuinfo`
if [ `grep -c JeVois /proc/cpuinfo` -gt 0 ]; then ncpu=4; fi

sudo /bin/rm -rf ppdbuild \
    && mkdir ppdbuild \
    && cd ppdbuild \
    && cmake "$@" -DJEVOISPRO_PLATFORM_DEB=ON -DJEVOIS_HARDWARE=PRO -DJEVOIS_PLATFORM=ON .. \
    && make -j ${ncpu} \
    && sudo make install
