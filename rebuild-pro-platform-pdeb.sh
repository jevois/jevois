#!/bin/sh
# USAGE: rebuild-pro-platform-pdeb.sh [cmake opts]

# compiled code is in /var/lib/jevoispro-build-pdeb/

# Get the external contributed packages if they are not here or are outdated:
./Contrib/check.sh

sudo /bin/rm -rf ppdbuild \
    && mkdir ppdbuild \
    && cd ppdbuild \
    && cmake "$@" -DJEVOISPRO_PLATFORM_DEB=ON -DJEVOIS_HARDWARE=PRO -DJEVOIS_PLATFORM=ON .. \
    && make -j \
    && sudo make install \
    && sudo cpack

