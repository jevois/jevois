#!/bin/bash

# Cleanup /jevois, /var/lib/jevois-microsd, /var/lib/jevois-build and more

read -p "Do you want to delete all files in /jevois, /var/lib/jevois-microsd, /var/lib/jevois-build/usr and more. This will require that you re-run rebuild-host.sh and rebuild-platform.sh in jevois, jevoisbase, and your custom modules (you will not need to recompile the Linux kernel or buildroot) [y/N]? "
if [ "X$REPLY" = "Xy" ]; then
    sudo rm -rf /jevois
    sudo rm -rf /var/lib/jevois-microsd/*
    sudo rm -rf /var/lib/jevois-build/usr
    sudo rm -f /usr/bin/jevois-*
    sudo rm -rf /usr/include/jevois
    sudo rm -rf /usr/include/jevoisbase
    sudo rm -rf /usr/lib/libjevois*
fi
