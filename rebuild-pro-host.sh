#!/bin/sh

# On JeVoisPro, limit the number of compile threads to not run out of memory:
ncpu=`grep -c processor /proc/cpuinfo`
if [ `grep -c JeVois /proc/cpuinfo` -gt 0 ]; then ncpu=4; fi
   
sudo /bin/rm -rf phbuild \
    && mkdir phbuild \
    && cd phbuild\
    && cmake "$@" -DJEVOIS_HARDWARE=PRO .. \
    && make -j ${ncpu} \
    && sudo make install
