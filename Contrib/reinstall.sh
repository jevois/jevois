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
if [ "x$1" = "x-y" ]; then
    REPLY="y";
else			   
    read -p "Do you want to nuke, fetch and patch contributed packages [y/N]? "
fi


if [ "X$REPLY" = "Xy" ]; then
    ###################################################################################################
    # Cleanup:
    /bin/rm -rf tensorflow pycoral threadpool tflite include lib

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
    get_github tensorflow tensorflow ${tc//\"/}

    # C++20 thread pool (we actually implement our own ThreadPool.H/C but need the dependencies):
    get_github mzjaworski threadpool f45dab47af20247949ebc43b429c742ef4c1219f
    
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

    # We need bazel-3.7.2:
    bzl="bazel-3.7.2"
    if [ ! -x /usr/bin/${bzl} ]; then
        echo "### JeVois: Installing ${bzl} ..."
        sudo apt -y install apt-transport-https curl gnupg
        curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
        sudo mv bazel.gpg /etc/apt/trusted.gpg.d/
        echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | \
            sudo tee /etc/apt/sources.list.d/bazel.list
        sudo apt update
        sudo apt -y install bazel-3.7.2
    fi
        
    # Build for host:
    echo "### JeVois: compiling tensorflow for host ..."
    ${bzl} build -c opt //tensorflow/lite:libtensorflowlite.so
    sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so ../lib/amd64/

    # Copy some includes:
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

    cd ..

    ###################################################################################################
    # ONNX Runtime for C++: need to download tarballs from github
    # In our CMakeLists.txt we include the onnxruntime includes and libs into the jevois deb
    ORT_VER="1.15.1"

    # For host:
    wget https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/onnxruntime-linux-x64-${ORT_VER}.tgz
    tar xvf onnxruntime-linux-x64-${ORT_VER}.tgz
    /bin/rm onnxruntime-linux-x64-${ORT_VER}.tgz
    mkdir -p include/amd64/onnxruntime
    /bin/cp -a onnxruntime-linux-x64-${ORT_VER}/include/* include/amd64/onnxruntime/
    /bin/cp onnxruntime-linux-x64-${ORT_VER}/lib/libonnxruntime.so lib/amd64/ # no symlinks allowed
    /bin/rm -rf onnxruntime-linux-x64-${ORT_VER}
    
    # For jevois-pro platform:
    wget https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/onnxruntime-linux-aarch64-${ORT_VER}.tgz
    tar xvf onnxruntime-linux-aarch64-${ORT_VER}.tgz
    /bin/rm onnxruntime-linux-aarch64-${ORT_VER}.tgz
    mkdir -p include/arm64/onnxruntime
    /bin/cp -a onnxruntime-linux-aarch64-${ORT_VER}/include/* include/arm64/onnxruntime/
    /bin/cp onnxruntime-linux-aarch64-${ORT_VER}/lib/libonnxruntime.so lib/arm64/ # no symlinks allowed
    /bin/rm -rf onnxruntime-linux-aarch64-${ORT_VER}
    
    # pycoral build
    #cd pycoral
    #./scripts/build.sh
    #make wheel
    #cd ..
       
    ###################################################################################################
    # Keep track of the last installed release:
    echo $release > .installed
    echo "JeVois contribs installation success."
fi
