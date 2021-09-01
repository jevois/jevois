#!/bin/sh

# Just print a list of all source files to stdout, for use in other scripts.

flz=$(find `dirname $0`/.. \
    \! -path \*build\* -a \
    \! -path \*Contrib\* -a \
    \( -name \*.H \
    -o -name \*.C \
    -o -name \*.cc \
    -o -name \*.hh \
    -o -name \*.c \
    -o -name \*.h \
    -o -name \*.hpp \
    -o -name \*.cpp \) )

for f in $flz; do
    g=${f#`dirname $0`/../}
    echo $g
done
