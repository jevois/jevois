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

# patched imgui is already added to the jevois tree, we will not patch it further:
/bin/rm -f imgui.patch

echo "All done."
