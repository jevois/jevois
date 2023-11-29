#!/bin/sh

set -e

# Get the external contributed packages if they are not here or are outdated:
./Contrib/check.sh

sudo /bin/rm -rf phbuild \
    && mkdir phbuild \
    && cd phbuild\
    && cmake "$@" -DJEVOIS_HARDWARE=PRO .. \
    && make -j \
    && sudo make install \
    && sudo ldconfig
        
