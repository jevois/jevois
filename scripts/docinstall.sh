#!/bin/sh

# This script only works on Prof Itti's computer, do not use it!
dest=/lab/jevois
#dest=/home2/tmp/u/jvdoc # for testing before official release


/bin/rm -rf ${dest}/*

cd doc/html
tar cvf - . | ( cd ${dest}/doc; tar xf - )

cd ..
cp modstyle.css ${dest}/

cd /lab/itti/jevois/software/jevoisbase/src/Modules
mkdir -p ${dest}/moddoc
tar cvf - */modinfo.html */screenshot* */icon* */video* | ( cd ${dest}/moddoc; tar xf - )

cd ${dest}/moddoc


echo "<table><tr><td>" > ../moddoc.inc
for m in `/bin/ls`; do
    grep -B 3 modinfosynopsis $m/modinfo.html >> ../moddoc.inc
    echo "<tr><td>&nbsp;</td></tr>" >> ../moddoc.inc
done
echo "</table>" >> ../moddoc.inc

cd ../doc/

sed -i -e '/click/r ../moddoc.inc' UserDemos.html
