#!/bin/bash

for d in `/bin/ls -p1 | grep /`; do
    if [ -d "${d}/.git" ]; then
	echo "Update patch for $d ..."
	cd $d
	patchfile="../${d%/}.patch"
	git diff > ${patchfile}
	if [ ! -s ${patchfile} ]; then /bin/rm ${patchfile}; fi
	cd ..
    fi
done
echo "All done."
