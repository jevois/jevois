#!/bin/sh

##############################################################################################################
# Default settings:
##############################################################################################################

CAMERA=ov9650
if [ -f /boot/sensor ]; then CAMERA=$(tail -1 /boot/sensor | tr -d '\r\n'); fi

use_usbserial=1    # Allow using a serial-over-USB to communicate with JeVois command-line interface
use_usbsd=1        # Allow exposing the JEVOIS partition of the microSD as a USB drive
use_serialtty=0    # Use a TTY on the hardware serial and do not use it in jevois-daemon
use_usbserialtty=0 # Use a TTY on the serial-over-USB and do not use it in jevois-daemon
use_maxbandwidth=1 # Use 100% of isochronous bandwidth to minimize latency; disable to connect several cameras
use_quietcmd=0     # Do not say OK after every good command when true
use_nopython=0     # Disable python to save memory when true

if [ -f /boot/nousbserial ]; then use_usbserial=0; echo "JeVois serial-over-USB disabled"; fi
if [ -f /boot/nousbsd ]; then use_usbsd=0; echo "JeVois microSD access over USB disabled"; fi
if [ -f /boot/multicam ]; then use_maxbandwidth=0; echo "JeVois multi-camera mode enabled"; fi
if [ -f /boot/quietcmd ]; then use_quietcmd=1; echo "JeVois quiet command-line mode enabled"; fi
if [ -f /boot/nopython ]; then use_nopython=1; echo "JeVois python support disabled"; fi

# Block device we present to the host as a USB drive, or empty to not present it at start:
usbsdfile=""
if [ -f /boot/usbsdauto ]; then usbsdfile="/dev/mmcblk0p3"; echo "JeVois microSD access over USB is AUTO"; fi

# Login prompts over hardware serial or serial-over-USB:
if [ -f /boot/serialtty ]; then use_serialtty=1; echo "Using tty on JeVois hardware serial"; fi
if [ "X${use_usbserial}" != "X1" ]; then use_usbserialtty=0;
elif [ -f /boot/usbserialtty ]; then use_usbserialtty=1; echo "Using tty on JeVois serial-over-USB"; fi

##############################################################################################################
# Load all required kernel modules:
##############################################################################################################

cd /lib/modules/3.4.39

for m in videobuf-core videobuf-dma-contig videodev vfe_os vfe_subdev v4l2-common v4l2-int-device \
		       cci vfe_v4l2 ump disp mali ; do
    if [ $m = "vfe_v4l2" ]; then
        # Try to detect the sensor. Start with the one that was selected, and, if that fails, try all others:
        for s in ${CAMERA} ov7725 ov2640 ar0135 ov9650; do
	        echo "### insmod ${s}.ko ###"
	        insmod ${s}.ko
	        echo "### insmod ${m}.ko sensor=${s} ###"
	        insmod ${m}.ko sensor="${s}"
            if [ $? -eq 0 ]; then CAMERA="${s}"; break; fi
	        echo "### rmmod ${s}.ko ###"
            rmmod ${s}.ko
        done
    else
	    echo "### insmod ${m}.ko ###"
	    insmod ${m}.ko
    fi
done

##############################################################################################################
# Install any new packages:
##############################################################################################################

cd /jevois
for f in packages/*.jvpkg; do
    if [ -f "${f}" ]; then
	echo "### Installing package ${f} ###"
	bzcat "${f}" | tar xvf -
	sync
	rm -f "${f}"
	sync
    fi
done

##############################################################################################################
# Find any newly unpacked postinstall scripts, run them, and delete them:
##############################################################################################################

for f in modules/*/*/postinstall; do
    if [ -f "${f}" ]; then
	echo "### Running ${f} ###"
	d=`dirname "${f}"`
	cd "${d}"
	sh postinstall
	sync
	rm -f postinstall
	sync
	cd /jevois
    fi
done

##############################################################################################################
# Build a default videomappings.cfg, if missing:
##############################################################################################################

if [ ! -f /jevois/config/videomappings.cfg ]; then
    echo 'YUYV 640 360 30.0 YUYV 320 240 30.0 JeVois JeVoisIntro *' > /jevois/config/videomappings.cfg
    echo 'YUYV 640 480 30.0 YUYV 320 240 30.0 JeVois JeVoisIntro' >> /jevois/config/videomappings.cfg
fi

##############################################################################################################
# Get a list of all our needed library paths:
##############################################################################################################

LIBPATH="/lib:/usr/lib"
for d in `find /jevois/lib -type d -print`; do LIBPATH="${LIBPATH}:${d}"; done
export LD_LIBRARY_PATH=${LIBPATH}

##############################################################################################################
# Insert the gadget driver:
##############################################################################################################

echo "### Insert gadget driver ###"
MODES=`/usr/bin/jevois-module-param ${CAMERA}`

insmodopts=""
if [ "X${use_usbsd}" = "X1" ]; then insmodopts="${insmodopts} file=${usbsdfile}"; fi

insmod /lib/modules/3.4.39/g_jevoisa33.ko modes=${MODES} use_serial=${use_usbserial} \
       use_storage=${use_usbsd} max_bandwidth=${use_maxbandwidth} ${insmodopts}

##############################################################################################################
# Launch jevois-daemon:
##############################################################################################################

echo "### Start jevois daemon ###"
opts=""
if [ "X${use_usbserial}" != "X1" -o "X${use_usbserialtty}" = "X1" ]; then opts="${opts} --usbserialdev="; fi
if [ "X${use_serialtty}" = "X1" ]; then opts="${opts} --serialdev="; fi
if [ "X${use_maxbandwidth}" != "X1" ]; then opts="${opts} --multicam=1"; fi
if [ "X${use_quietcmd}" = "X1" ]; then opts="${opts} --quietcmd=1"; fi
if [ "X${use_nopython}" = "X1" ]; then opts="${opts} --python=0"; fi
if [ "X${CAMERA}" != "X" ]; then opts="${opts} --camerasens=${CAMERA}"; fi

# Start the jevois daemon:
if [ "X${use_serialtty}" = "X1" -o "X${use_usbserialtty}" = "X1" ]; then
    /usr/bin/jevois-daemon ${opts} &
else
    # If you have not specified any special files in /boot other than possibly "login", this is where you would launch
    # jevois-daemon in gdb for debugging hard crashes using a USB-to-serial cable connected to the hardware serial port
    # of JeVois. Note that since the inclusion of Python support, libcrypto gets pulled in, which triggers some illegal
    # instruction exception caught by gdb. This is a known issue. Type this in gdb to suppress these errors before you
    # run the program: "handle SIGILL nostop noprint" and see https://www.raspberrypi.org/forums/viewtopic.php?p=155085
    /usr/bin/jevois-daemon ${opts}
    # gdb --args /usr/bin/jevois-daemon ${opts}
fi

##############################################################################################################
# Launch any ttys:
##############################################################################################################
if [ "X${use_usbserialtty}" = "X1" ]; then
    while [ ! -c /dev/ttyGS0 ]; do sleep 1; done # wait until gadget is operational
    if [ "X${use_serialtty}" = "X1" ]; then
	    /sbin/getty -L ttyGS0 115200 vt100 &
    else
	    /sbin/getty -L ttyGS0 115200 vt100
    fi
fi

if [ "X${use_serialtty}" = "X1" ]; then
    /sbin/getty -L ttyS0 115200 vt100
fi
