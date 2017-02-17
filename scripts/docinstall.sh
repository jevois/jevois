#!/bin/sh

# This script only works on Prof Itti's computer, do not use it!

/bin/rm -rf /lab/jevois/doc/*

cd doc/html
tar cvf - . | ( cd /lab/jevois/doc; tar xf - )

cd ..
cp modstyle.css /lab/jevois/

cd /lab/itti/jevois/software/jevoisbase/src/Modules
mkdir -p /lab/jevois/moddoc
tar cvf - */modinfo.html */screenshot* */icon* */video* | ( cd /lab/jevois/moddoc; tar xf - )

cd /lab/jevois/moddoc


echo "<table><tr><td>" > ../moddoc.inc
for m in `/bin/ls`; do
    grep -B 3 modinfosynopsis $m/modinfo.html >> ../moddoc.inc
    echo "<tr><td>&nbsp;</td></tr>" >> ../moddoc.inc
done
echo "</table>" >> ../moddoc.inc

cd ../doc/

sed -i -e '/click/r ../moddoc.inc' UserDemos.html
