#!/bin/sh
# USAGE: rebuild-pro-platform.sh [cmake opts]

# compiled code is in /var/lib/jevoispro-build/ and /var/lib/jevoispro-microsd/

set -e

# Get the external contributed packages if they are not here or are outdated:
./Contrib/check.sh

sudo /bin/rm -rf ppbuild \
    && mkdir ppbuild \
    && cd ppbuild \
    && cmake "$@" -DJEVOIS_HARDWARE=PRO -DJEVOIS_PLATFORM=ON .. \
    && make -j \
    && sudo make install
