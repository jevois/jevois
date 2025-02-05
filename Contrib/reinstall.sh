#!/bin/bash
# usage: reinstall.sh [-y]
# will nuke and re-install all contributed packages
#
# Convention for libs and includes:
# - includes go into include/amd64 (for host), include/armhf (jevois-a33), include/arm64 (jevois-pro),
#   or include/all (for all). Only add directories to to include/*
# - libs go into lib/amd64 (for host), lib/armhf (jevois-a33), lib/arm64 (jevois-pro). Only add files and
#   symlinks to lib/*
#
# The JeVois cmake will then install them into the appropriate /jevois[pro]/include and /jevois[pro]/lib

set -e # Exit on any error

# Go to the Contrib directory (where this script is):
cd "$( dirname "${BASH_SOURCE[0]}" )"

# Bump this release number each time you make significant changes here, this will cause rebuild-host.sh to re-run
# this reinstall script:
release=`cat RELEASE`

###################################################################################################
function finish
{
    rc=$?
    if [ ! -f .installed ] || (( `cat .installed` < `cat RELEASE` )); then echo "--- ABORTED on error"; fi
    exit $rc
}

trap finish EXIT

###################################################################################################
function get_github # owner, repo, revision
{
    echo "### JeVois: downloading ${1} / ${2} ..."
    git clone --recursive --recurse-submodules "https://github.com/${1}/${2}.git"
    if [ "X${3}" != "X" ]; then
        echo "### JeVois: moving ${1} / ${2} to checkout ${3} ..."
        cd "${2}"
        git checkout -q ${3}
        cd ..
    fi
}

###################################################################################################
function patchit # directory
{
    if [ ! -d ${1} ]; then
	    echo "Ooops cannot patch ${1} because directory is missing";
    else
        echo "### JeVois: patching ${1} ..."
	    cd ${1}
	    patch -p1 < ../${1}.patch
	    cd ..
    fi
}

###################################################################################################
if [ "x$1" = "x-y" ]; then usedef=1; else usedef=0; fi
function question { if [ $usedef -eq 1 ]; then REPLY="y"; else read -p "JEVOIS: ${1}? [Y/n] "; fi }

###################################################################################################
question "Nuke, fetch and patch contributed packages"
if [ "X$REPLY" = "Xn" ]; then
    echo "Aborted."
    exit 1
fi

###################################################################################################
# Cleanup:
/bin/rm -rf tensorflow threadpool tflite include lib clip.cpp

###################################################################################################
# Get the jevois version:
ma=`grep "set(JEVOIS_VERSION_MAJOR" ../CMakeLists.txt | awk '{ print $2 }' | sed -e 's/)//' `
mi=`grep "set(JEVOIS_VERSION_MINOR" ../CMakeLists.txt | awk '{ print $2 }' | sed -e 's/)//' `
pa=`grep "set(JEVOIS_VERSION_PATCH" ../CMakeLists.txt | awk '{ print $2 }' | sed -e 's/)//' `
ver="${ma}.${mi}.${pa}"

###################################################################################################
question "Download pre-compiled binaries for JeVois ${ver} instead of rebuilding from source"
if [ "X$REPLY" != "Xn" ]; then
    wget http://jevois.org/data/contrib-binary-${ver}.tbz
    tar jxvf contrib-binary-${ver}.tbz
    /bin/rm -f contrib-binary-${ver}.tbz
    echo "All done."
    trap - EXIT
    exit 0
fi

# If we make it here, we will rebuild from source:
mkdir -p include/amd64 include/armhf include/arm64 include/all
mkdir -p lib/amd64 lib/armhf lib/arm64

###################################################################################################
# Get the packages:

# Google coral TPU: libedgetpu, libcoral, and pycoral are all in the pycoral repo:
#get_github google-coral pycoral 08ad3209497518e8a7d5df863ce51d5b47af7d82

# Tensorflow, needed for libtensorflowlite.so and includes, not available as deb package.  Tensorflow version must
# exactly match that used for libedgetpu, specified as TENSORFLOW_COMMIT in pycoral/WORKSPACE
#tc=`grep ^TENSORFLOW_COMMIT pycoral/workspace.bzl |awk '{ print \$3 }'`
#tc="48c3bae94a8b324525b45f157d638dfd4e8c3be1" # version used by frogfish tpu release
tc="a4dfb8d1a71385bd6d122e4f27f86dcebb96712d" # TF 2.5.0, for use with grouper tpu release
#tc="5bc9d26649cca274750ad3625bd93422617eed4b" # TF 2.16.1, fo ruse with jevois 1.21.0 and compiled libcoral
get_github tensorflow tensorflow ${tc//\"/}

# C++20 thread pool (we actually implement our own ThreadPool.H/C but need the dependencies):
get_github mzjaworski threadpool f45dab47af20247949ebc43b429c742ef4c1219f

# clip.cpp for CLIP text and image embedding
get_github monatis clip.cpp f4ee24b

###################################################################################################
# Patching:
for f in *.patch; do
	patchit ${f/.patch/}
done

###################################################################################################
# threadpool: just the includes
/bin/cp -arv threadpool/include/threadpool include/all/
/bin/cp -arv threadpool/libs/function2/include/function2 include/all/
mkdir include/all/concurrentqueue
/bin/cp -av threadpool/libs/concurrentqueue/*.h include/all/concurrentqueue/

###################################################################################################
# Tensorflow dependencies and build:
cd tensorflow
./tensorflow/lite/tools/make/download_dependencies.sh

# Patch downloaded deps:
sed -i '/#include <cstdint>/a #include <limits>' \
    tensorflow/lite/tools/make/downloads/ruy/ruy/block_map.cc

sed -i '/#include <limits.h>/a #include <cstdint>' \
    tensorflow/lite/tools/make/downloads/absl/absl/strings/internal/str_format/extension.h

# We need bazel-3.7.2 to compile tensorflow:
wget http://jevois.org/data/bazel_3.7.2-linux-x86_64.deb
sudo dpkg -i bazel_3.7.2-linux-x86_64.deb
/bin/rm -f bazel_3.7.2-linux-x86_64.deb
bzl="bazel-3.7.2"

# Build for host:
echo "### JeVois: compiling tensorflow for host ..."
${bzl} build -c opt //tensorflow/lite:libtensorflowlite.so || true

echo "\n\n\nJEVOIS: no worries, we will fix that error now...\n\n\n"

for f in `find ~/.cache/bazel -name block_map.cc`; do sed -i '/#include <cstdint>/a #include <limits>' $f; done
for f in `find ~/.cache/bazel -name extension.h`; do sed -i '/#include <limits.h>/a #include <cstdint>' $f; done
for f in `find ~/.cache/bazel -name spectrogram.h`; do sed -i '/#include <vector>/a #include <cstdint>' $f; done

${bzl} build -c opt //tensorflow/lite:libtensorflowlite.so || true
sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so ../lib/amd64/

# Copy some includes that we may need when compiling our code:
/usr/bin/find tensorflow/lite -name '*.h' -print0 | \
    tar cvf - --null --files-from - | \
    ( cd ../include/all/ && tar xf - )

# Build for JeVois-Pro platform:
echo "### JeVois: cross-compiling tensorflow for JeVois-Pro platform ..."
${bzl} build --config=elinux_aarch64 -c opt //tensorflow/lite:libtensorflowlite.so
sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so ../lib/arm64/

# Build for JeVois-A33 platform:
echo "### JeVois: cross-compiling tensorflow for JeVois-A33 platform ..."
${bzl} build --config=elinux_armhf -c opt //tensorflow/lite:libtensorflowlite.so
sudo mkdir -p /var/lib/jevois-microsd/lib
sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so ../lib/armhf/

# Build wheel for host: currently disabled as we use the pycoral wheels instead
## We need docker installed
#docker --version || ( echo "\n\n\nJEVOIS: Please install docker to compile tensorflow -- ABORT"; exit 1 )
#cd tensorflow/lite/tools/pip_package/
#make BASE_IMAGE=ubuntu:24.04 PYTHON=python3 TENSORFLOW_TARGET=k8 docker-build
#mv out/python3/ubuntu*/tflite_runtime*.whl ../../../../../
## Build wheel for platform:
#make BASE_IMAGE=ubuntu:24.04 PYTHON=python3 TENSORFLOW_TARGET=aarch64 docker-build
#mv out/python3/ubuntu*/tflite_runtime*.whl ../../../../../
#cd ../../../..

cd ..

###################################################################################################
# ONNX Runtime for C++: need to download tarballs from github
# In our CMakeLists.txt we include the onnxruntime includes and libs into the jevois deb
ORT_VER="1.18.0"

# For host:
wget https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/onnxruntime-linux-x64-${ORT_VER}.tgz
tar xvf onnxruntime-linux-x64-${ORT_VER}.tgz
/bin/rm onnxruntime-linux-x64-${ORT_VER}.tgz
mkdir -p include/amd64/onnxruntime
/bin/cp -a onnxruntime-linux-x64-${ORT_VER}/include/* include/amd64/onnxruntime/
/bin/cp onnxruntime-linux-x64-${ORT_VER}/lib/libonnxruntime.so lib/amd64/libonnxruntime.so.${ORT_VER}
/bin/rm -rf onnxruntime-linux-x64-${ORT_VER}

# For jevois-pro platform:
wget https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/onnxruntime-linux-aarch64-${ORT_VER}.tgz
tar xvf onnxruntime-linux-aarch64-${ORT_VER}.tgz
/bin/rm onnxruntime-linux-aarch64-${ORT_VER}.tgz
mkdir -p include/arm64/onnxruntime
/bin/cp -a onnxruntime-linux-aarch64-${ORT_VER}/include/* include/arm64/onnxruntime/
/bin/cp onnxruntime-linux-aarch64-${ORT_VER}/lib/libonnxruntime.so lib/arm64/libonnxruntime.so.${ORT_VER}
/bin/rm -rf onnxruntime-linux-aarch64-${ORT_VER}

###################################################################################################
# Keep track of the last installed release:
echo $release > .installed
echo "JeVois contribs installation success."
trap - EXIT
