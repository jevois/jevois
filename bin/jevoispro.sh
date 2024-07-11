#!/bin/bash

# Uncoment this if you want to get a core dump. After a crash, run: gdb /usr/bin/jevoispro-daemon core
# and after you get to the location of the crash, type "bt" to get the backtrace that led to it.
#
#ulimit -c unlimited

extra=""

# Check whether we want to enable g_serial:
gserial=`cat /.jevoispro_use_gserial 2>/dev/null`
if [ "X${gserial}" = "X1" ] ; then
    modprobe g_serial
    extra="${extra} --usbserialdev=/dev/ttyGS0"
fi

# Activate python virtual environment if found:
if [ "X$VIRTUAL_ENV" = "X" -a -d /root/jvenv ]; then source /root/jvenv/bin/activate; fi

# Launch jevois software:
/usr/bin/jevoispro-daemon ${extra} $@
rc=$?

# Restore the console, which was disabled by jevoispro-daemon:
/usr/bin/jevoispro-restore-console

# Deactivate virtual env if we used it:
if [ "X$VIRTUAL_ENV" != "X" ]; then deactivate; fi

# Return whatever jevoispro-daemon returned:
exit $rc
