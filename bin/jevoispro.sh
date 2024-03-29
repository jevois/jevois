#!/bin/sh

cd /root

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

# Launch jevois software:
/usr/bin/jevoispro-daemon ${extra} $@
rc=$?

# Restore the console, which was disabled by jevois:
/usr/bin/jevoispro-restore-console

# Return whatever jevoispro-daemon returned:
exit $rc
