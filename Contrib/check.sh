#!/bin/bash

# Go to the Contrib directory (where this script is):
cd "$( dirname "${BASH_SOURCE[0]}" )"

# Get the external contributed packages if they are not here or are outdated:
if [ ! -f .installed ] || (( `cat .installed` < `cat RELEASE` )); then
    echo "Contrib packages are missing or outdated. Running Contrib/reinstall.sh ..."
    ./reinstall.sh
fi
