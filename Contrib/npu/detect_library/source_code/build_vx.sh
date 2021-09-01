#!/bin/bash

set -e

########################################################
# establish build environment and build options value
# Please modify the following items according your build environment

if [ -z "$1" ]; then
	echo "usage: $0 <linux sdk dir>"
	exit 1
fi


export AQROOT=$1
#export AQARCH=$AQROOT/arch/XAQ2
export SDK_DIR=$AQROOT/build/sdk
export OPENCV_ROOT=$SDK_DIR/opencv3

export VIVANTE_SDK_DIR=$SDK_DIR
export VIVANTE_SDK_INC=$SDK_DIR/include
export VIVANTE_SDK_LIB=$SDK_DIR/drivers
export OVXLIB_DIR=$AQROOT/acuity-ovxlib-dev


ARCH=arm64
export ARCH_TYPE=$ARCH
export CPU_TYPE=cortex-a53
export CPU_ARCH=armv8-a
export FIXED_ARCH_TYPE=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
export TOOLCHAIN=$AQROOT/../../toolchains/gcc-linaro-aarch64-linux-gnu/bin
export LIB_DIR=$TOOLCHAIN/../aarch64-linux-gnu/libc/lib
########################################################
# set special build options valule
# You can modify the build options for different results according your requirement
#
#    option                    value   description                          default value
#    -------------------------------------------------------------------------------------
#    DEBUG                      1      Enable debugging.                               0
#                               0      Disable debugging.
#
#    NO_DMA_COHERENT            1      Disable coherent DMA function.                  0
#                               0      Enable coherent DMA function.
#
#                                      Please set this to 1 if you are not sure what
#                                      it should be.
#
#    ABI                        0      Change application binary interface, default    0
#                                      is 0 which means no setting
#                                      aapcs-linux For example, build driver for Aspenite board
#
#    LINUX_OABI                 1      Enable this if build environment is ARM OABI.   0
#                               0      Normally disable it for ARM EABI or other machines.
#
#    USE_VDK                    1      Eanble this one when the applications           0
#                                      are using the VDK programming interface.
#                               0      Disable this one when the applications
#                                      are NOT using the VDK programming interface.
#
#                                      Don't eanble gcdSTATIC_LINK (see below)
#                                      at the same time since VDK will load some
#                                      libraries dynamically.
#
#    EGL_API_FB                 1      Use the FBDEV as the GUI system for the EGL.    0
#                               0      Use X11 system as the GUI system for the EGL.
#
#    EGL_API_DRI                1      Use DRI to support X accelerator.               0
#                                      EGL_API_FB and EGL_API_DFB must be 0.
#                               0      Do not use DRI to support X accelerator.
#
#    X11_DRI3                   1      Use DRI3 framework to support X11.              0
#                               0      Do not use DRI3 framework to support X11.
#
#    EGL_API_DFB                1      Use directFB accelerator.                       0
#                                      EGL_API_FB and EGL_API_DRI must be 0.
#                               0      Do not use directFB accelerator.
#
#    gcdSTATIC_LINK             1      Enable static linking.                          0
#                               0      Disable static linking;
#
#                                      Don't enable this one when you are building
#                                      GFX driver and HAL unit tests since both of
#                                      them need dynamic linking mechanisim.
#                                      And it must NOT be enabled when USE_VDK=1.
#
#   CUSTOM_PIXMAP               1      Use the user defined Pixmap format in EGL.      0
#                               0      Use X11 pixmap format in EGL.
#
#    ENABLE_GPU_CLOCK_BY_DRIVER 1      Set the GPU clock in the driver.                0
#                               0      The GPU clock should be set by BSP.
#
#    USE_FB_DOUBLE_BUFFER       0      Use single buffer for the FBDEV                 0
#                               1      Use double buffer for the FBDEV (NOTE: the FBDEV must support it)
#
#
#    USE_PLATFORM_DRIVER        1      Use platform driver model to build kernel       1
#                                      module on linux while kernel version is 2.6.
#                               0      Use legecy kernel driver model.
#
#    FPGA_BUILD                 1      Set this option to 1 if you're running on       0
#                                      FPGA board.
#                               0
#
#    USE_BANK_ALIGNMENT         1      Enable gcdENABLE_BANK_ALIGNMENT, and video memory is allocated bank aligned.
#                                      The vendor can modify _GetSurfaceBankAlignment() and gcoSURF_GetBankOffsetBytes()
#                                      to define how different types of allocations are bank and channel aligned.
#                               0      Disabled (default), no bank alignment is done.
#
#    BANK_BIT_START             0      Use default start bit of the bank defined in gc_hal_options.h
#                    [BANK_BIT_START]  Specifies the start bit of the bank (inclusive).
#                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;
#
#    BANK_BIT_END               0      Use default end bit of the bank defined in gc_hal_options.h
#                    [BANK_BIT_END]    Specifies the end bit of the bank (inclusive);
#                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled;
#
#    BANK_CHANNEL_BIT           0      Use default channel bit defined in gc_hal_options.h
#                  [BANK_CHANNEL_BIT]  When set to no-zero, video memory when allocated bank aligned is allocated such
#                                      that render and depth buffer addresses alternate on the channel bit specified.
#                                      This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled.
#

BUILD_OPTION_DEBUG=0
BUILD_OPTION_ABI=0
BUILD_OPTION_LINUX_OABI=0
BUILD_OPTION_NO_DMA_COHERENT=0
BUILD_OPTION_USE_VDK=1
if [ -z $BUILD_OPTION_EGL_API_FB ]; then
    BUILD_OPTION_EGL_API_FB=1
fi
if [ -z $BUILD_OPTION_EGL_API_DFB ]; then
    BUILD_OPTION_EGL_API_DFB=0
fi
if [ -z $BUILD_OPTION_EGL_API_DRI ]; then
    BUILD_OPTION_EGL_API_DRI=0
fi
if [ -z $BUILD_OPTION_X11_DRI3 ]; then
    BUILD_OPTION_X11_DRI3=0
fi
if [ -z $BUILD_OPTION_EGL_API_WL ]; then
    BUILD_OPTION_EGL_API_WL=0
fi
if [ -z $BUILD_OPTION_EGL_API_NULLWS ]; then
    BUILD_OPTION_EGL_API_NULLWS=0
fi
BUILD_OPTION_gcdSTATIC_LINK=0
BUILD_OPTION_CUSTOM_PIXMAP=0
BUILD_OPTION_USE_3D_VG=1
if [ -z $BUILD_OPTION_USE_OPENCL ]; then
    BUILD_OPTION_USE_OPENCL=0
fi
if [ -z $BUILD_OPTION_USE_OPENVX ]; then
    BUILD_OPTION_USE_OPENVX=1
fi
if [ -z $BUILD_OPTION_USE_VULKAN ]; then
    BUILD_OPTION_USE_VULKAN=0
fi
BUILD_OPTION_USE_FB_DOUBLE_BUFFER=0
BUILD_OPTION_USE_PLATFORM_DRIVER=1
if [ -z $BUILD_OPTION_ENABLE_GPU_CLOCK_BY_DRIVER ]; then
    BUILD_OPTION_ENABLE_GPU_CLOCK_BY_DRIVER=0
fi

if [ -z $BUILD_YOCTO_DRI_BUILD ]; then
    BUILD_YOCTO_DRI_BUILD=0
fi

BUILD_OPTION_CONFIG_DOVEXC5_BOARD=0
BUILD_OPTION_FPGA_BUILD=0
BUILD_OPTIONS="NO_DMA_COHERENT=$BUILD_OPTION_NO_DMA_COHERENT"
BUILD_OPTIONS="$BUILD_OPTIONS USE_VDK=$BUILD_OPTION_USE_VDK"
BUILD_OPTIONS="$BUILD_OPTIONS EGL_API_FB=$BUILD_OPTION_EGL_API_FB"
BUILD_OPTIONS="$BUILD_OPTIONS EGL_API_DFB=$BUILD_OPTION_EGL_API_DFB"
BUILD_OPTIONS="$BUILD_OPTIONS EGL_API_DRI=$BUILD_OPTION_EGL_API_DRI"
BUILD_OPTIONS="$BUILD_OPTIONS X11_DRI3=$BUILD_OPTION_X11_DRI3"
BUILD_OPTIONS="$BUILD_OPTIONS EGL_API_NULLWS=$BUILD_OPTION_EGL_API_NULLWS"
BUILD_OPTIONS="$BUILD_OPTIONS gcdSTATIC_LINK=$BUILD_OPTION_gcdSTATIC_LINK"
BUILD_OPTIONS="$BUILD_OPTIONS EGL_API_WL=$BUILD_OPTION_EGL_API_WL"
BUILD_OPTIONS="$BUILD_OPTIONS ABI=$BUILD_OPTION_ABI"
BUILD_OPTIONS="$BUILD_OPTIONS LINUX_OABI=$BUILD_OPTION_LINUX_OABI"
BUILD_OPTIONS="$BUILD_OPTIONS DEBUG=$BUILD_OPTION_DEBUG"
BUILD_OPTIONS="$BUILD_OPTIONS CUSTOM_PIXMAP=$BUILD_OPTION_CUSTOM_PIXMAP"
BUILD_OPTIONS="$BUILD_OPTIONS USE_3D_VG=$BUILD_OPTION_USE_3D_VG"
BUILD_OPTIONS="$BUILD_OPTIONS USE_OPENCL=$BUILD_OPTION_USE_OPENCL"
BUILD_OPTIONS="$BUILD_OPTIONS USE_OPENVX=$BUILD_OPTION_USE_OPENVX"
BUILD_OPTIONS="$BUILD_OPTIONS USE_VULKAN=$BUILD_OPTION_USE_VULKAN"
BUILD_OPTIONS="$BUILD_OPTIONS USE_FB_DOUBLE_BUFFER=$BUILD_OPTION_USE_FB_DOUBLE_BUFFER"
BUILD_OPTIONS="$BUILD_OPTIONS USE_PLATFORM_DRIVER=$BUILD_OPTION_USE_PLATFORM_DRIVER"
BUILD_OPTIONS="$BUILD_OPTIONS ENABLE_GPU_CLOCK_BY_DRIVER=$BUILD_OPTION_ENABLE_GPU_CLOCK_BY_DRIVER"
BUILD_OPTIONS="$BUILD_OPTIONS CONFIG_DOVEXC5_BOARD=$BUILD_OPTION_CONFIG_DOVEXC5_BOARD"
BUILD_OPTIONS="$BUILD_OPTIONS FPGA_BUILD=$BUILD_OPTION_FPGA_BUILD"
BUILD_OPTIONS="$BUILD_OPTIONS YOCTO_DRI_BUILD=$BUILD_YOCTO_DRI_BUILD"
BUILD_OPTIONS="$BUILD_OPTIONS _GLIBCXX_USE_CXX11_ABI=1"

export PATH=$TOOLCHAIN:$PATH

make -f makefile.linux $BUILD_OPTIONS V_TARGET=clean
make -f makefile.linux $BUILD_OPTIONS V_TARGET=install  2>&1 | tee $AQROOT/linux_build_sample.log

########################################################
# other build/clean commands to build/clean specified items, eg.
#
# cd $AQROOT; make -f makefile.linux $BUILD_OPTIONS gal_core V_TARGET=clean || exit 1
# cd $AQROOT; make -f makefile.linux $BUILD_OPTIONS gal_core V_TARGET=install || exit 1

