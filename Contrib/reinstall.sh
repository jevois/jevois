#!/bin/bash
# usage: reinstall.sh [-y]
# will nuke and re-install all contributed packages

# Bump this release number each time you make significant changes here, this will cause rebuild-host.sh to re-run
# this reinstall script:
release=`cat RELEASE`

if [[ -z "${JEVOISPRO_SDK_ROOT}" ]]; then
    JEVOISPRO_SDK_ROOT=/usr/share/jevoispro-sdk
    echo "JEVOISPRO_SDK_ROOT is not set, using ${JEVOISPRO_SDK_ROOT}"
fi

if [[ -z "${JEVOIS_SDK_ROOT}" ]]; then
    JEVOIS_SDK_ROOT=/usr/share/jevois-sdk
    echo "JEVOIS_SDK_ROOT is not set, using ${JEVOIS_SDK_ROOT}"
fi

JEVOIS_BUILD_BASE="${JEVOIS_SDK_ROOT}/out/sun8iw5p1/linux/common/buildroot"
JEVOISPRO_BUILD_BASE="${JEVOISPRO_SDK_ROOT}/jevoispro-sysroot"

if [ ! -d "${JEVOIS_BUILD_BASE}" -a ! -d "${JEVOISPRO_BUILD_BASE}" ]; then
    echo "Cannot find either ${JEVOIS_BUILD_BASE} or ${JEVOISPRO_BUILD_BASE}"
    echo "You need to insall jevois-sdk-dev or jevoispro-sdk-dev first -- ABORT"
    exit 1
fi

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
    /bin/rm -rf tensorflow pycoral

    ###################################################################################################
    # Get the packages:

    # Google coral TPU: libedgetpu, libcoral, and pycoral are all in the pycoral repo:
    #get_github google-coral pycoral 08ad3209497518e8a7d5df863ce51d5b47af7d82
    
    # Tensorflow, needed for libtensorflowlite.so and includes, not available as deb package.  Tensorflow version must
    # exactly match that used for libedgetpu, specified as TENSORFLOW_COMMIT in pycoral/WORKSPACE
    #tc=`grep ^TENSORFLOW_COMMIT pycoral/WORKSPACE |awk '{ print \$3 }'`
    tc="48c3bae94a8b324525b45f157d638dfd4e8c3be1" # version used by frogfish release
    get_github tensorflow tensorflow ${tc//\"/}

    # C++20 thread pool (we actually implement our own ThreadPool.H/C but need the dependencies):
    get_github mzjaworski threadpool f45dab47af20247949ebc43b429c742ef4c1219f
    
    ###################################################################################################
    # Patching:
    for f in *.patch; do
	    patchit ${f/.patch/}
    done

    ###################################################################################################
    # Tensorflow dependencies and build:
    cd tensorflow
    ./tensorflow/lite/tools/make/download_dependencies.sh
    
    # Build for host:
    bazel build -c opt //tensorflow/lite:libtensorflowlite.so
    sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so /usr/lib/

    if [ -d "${JEVOISPRO_BUILD_BASE}" ]; then
        # Build for JeVois-Pro platform:
        bazel build --config=elinux_aarch64 -c opt //tensorflow/lite:libtensorflowlite.so
        sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so /var/lib/jevoispro-microsd/lib/ # for sd card
        sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so ${JEVOISPRO_BUILD_BASE}/usr/lib/ # for compiling
    fi
    
    if [ -d "${JEVOIS_BUILD_BASE}" ]; then
        # Build for JeVois-A33 platform:
        bazel build --config=elinux_armhf -c opt //tensorflow/lite:libtensorflowlite.so
        sudo mkdir -p /var/lib/jevois-microsd/lib
        sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so /var/lib/jevois-microsd/lib/ # for sd card
        sudo cp -v bazel-bin/tensorflow/lite/libtensorflowlite.so ${JEVOIS_BUILD_BASE}/target/usr/lib/ # for compiling
    fi

    cd ..
    
    # pycoral build
    #cd pycoral
    #./scripts/build.sh
    #make wheel
    #cd ..
       
    ###################################################################################################
    # Keep track of the last installed release:
    echo $release > .installed
fi
