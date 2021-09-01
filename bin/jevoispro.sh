#!/bin/sh

cd /root

# Uncoment this if you want to get a core dump. After a crash, run: gdb /usr/bin/jevoispro-daemon core
# and after you get to the location of the crash, type "bt" to get the backtrace that led to it.
#
#ulimit -c unlimited

# Launch jevois software:
/usr/bin/jevoispro-daemon $@
rc=$?

# Restore the console, which was disabled by jevois:
/usr/bin/jevoispro-restore-console

# Return whatever jevoispro-daemon returned:
exit $rc
