#!/bin/sh

# Get the external contributed packages if they are not here or are outdated:
./Contrib/check.sh

sudo /bin/rm -rf pbuild \
    && mkdir pbuild \
    && cd pbuild \
    && cmake "$@" -DJEVOIS_PLATFORM=ON .. \
    && make -j \
    && sudo make install
