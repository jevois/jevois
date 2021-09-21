#!/bin/sh

# This only works in iLab, it is to refresh and publish all docs to jevois.org

rm -rf $JEVOIS_SRC_ROOT/jevois/doc/html
rm -rf $JEVOIS_SRC_ROOT/jevoisbase/doc/html

# First, make the JeVois doc so we get the tagfile for cross-linking to jevoisbase:
cd $JEVOIS_SRC_ROOT/jevois
if [ ! -d hbuild ]; then ./rebuild-host.sh; fi
cd hbuild
make doc

# Then, extract modinfo doc for all modules:
cd $JEVOIS_SRC_ROOT/jevoisbase
rm -f src/Modules/*/modinfo*
if [ ! -d hbuild ]; then ./rebuild-host.sh; fi
cd hbuild
make -j

# Then make and publish the jevoisbase doc, which also creates its tagfile:
make docweb

# Make the jevois doc one more time and publish it:
cd $JEVOIS_SRC_ROOT/jevois
cd hbuild
make docweb

# Make and publish the tutorials:
cd $JEVOIS_SRC_ROOT/jevois-tutorials
make docweb

# install redirects
cat > /lab/jevois/doc/UserTutos.html <<EOF
<html><head><meta http-equiv="refresh" content="0; url=/tutorials/UserTutorials.html"></head><body></body></html>
EOF

cat > /lab/jevois/doc/ProgrammerTutos.html <<EOF
<html><head><meta http-equiv="refresh" content="0; url=/tutorials/ProgrammerTutorials.html"></head><body></body></html>
EOF

# Make and publish the blog:
cd $JEVOIS_SRC_ROOT/jevois-blog
make docweb
