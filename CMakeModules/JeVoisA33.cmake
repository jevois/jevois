######################################################################################################################
#
# JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
# California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
#
# This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
# redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
# Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.  You should have received a copy of the GNU General Public License along with this program;
# if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
# Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
######################################################################################################################

########################################################################################################################
# Python version to use for python modules, on host:
if (EXISTS "/usr/lib/x86_64-linux-gnu/libboost_python310.so")
  # Ubuntu 22.04 jammy
  set(JEVOIS_COMPILER_VERSION 10)
  set(JEVOIS_HOST_PYTHON_MAJOR 3)
  set(JEVOIS_HOST_PYTHON_MINOR 10)
  set(JEVOIS_HOST_PYTHON_M "")
  set(JEVOIS_HOST_BOOST_PYTHON "boost_python310")
  set(TURBOJPEG_PKG "libjpeg-turbo8-dev, libturbojpeg-dev")
elseif (EXISTS "/usr/lib/x86_64-linux-gnu/libboost_python38.so")
  # Ubuntu 20.04 focal
  set(JEVOIS_COMPILER_VERSION 10)
  set(JEVOIS_HOST_PYTHON_MAJOR 3)
  set(JEVOIS_HOST_PYTHON_MINOR 8)
  set(JEVOIS_HOST_PYTHON_M "")
  set(JEVOIS_HOST_BOOST_PYTHON "boost_python38")
  set(TURBOJPEG_PKG "libjpeg-turbo8-dev, libturbojpeg-dev")
elseif (EXISTS "/usr/lib/x86_64-linux-gnu/libboost_python3-py37.so")
  # Ubuntu 18.04 bionic
  set(JEVOIS_COMPILER_VERSION 7)
  set(JEVOIS_HOST_PYTHON_MAJOR 3)
  set(JEVOIS_HOST_PYTHON_MINOR 7)
  set(JEVOIS_HOST_PYTHON_M "m")
  set(JEVOIS_HOST_BOOST_PYTHON "boost_python3-py37")
  set(TURBOJPEG_PKG "libjpeg-turbo8-dev, libturbojpeg-dev")
elseif (EXISTS "/usr/lib/x86_64-linux-gnu/libboost_python3-py36.so")
  set(JEVOIS_COMPILER_VERSION 5)
  set(JEVOIS_HOST_PYTHON_MAJOR 3)
  set(JEVOIS_HOST_PYTHON_MINOR 6)
  set(JEVOIS_HOST_PYTHON_M "m")
  set(JEVOIS_HOST_BOOST_PYTHON "boost_python3-py36")
  set(TURBOJPEG_PKG "libjpeg-turbo8-dev, libturbojpeg-dev")
elseif (EXISTS "/usr/lib/x86_64-linux-gnu/libboost_python3-py35.so")
  set(JEVOIS_COMPILER_VERSION 5)
  set(JEVOIS_HOST_PYTHON_MAJOR 3)
  set(JEVOIS_HOST_PYTHON_MINOR 5)
  set(JEVOIS_HOST_PYTHON_M "m")
  set(JEVOIS_HOST_BOOST_PYTHON "boost_python3-py35")
  set(TURBOJPEG_PKG "libjpeg-turbo8-dev")
else()
  message(FATAL_ERROR "Cannot find libboost-python, please apt install libboost-all-dev")
endif()

########################################################################################################################
# Python to use on platform (this is fixed for a given JeVois software release):
set(JEVOIS_PLATFORM_PYTHON_MAJOR 3)
set(JEVOIS_PLATFORM_PYTHON_MINOR 7)
set(JEVOIS_PLATFORM_PYTHON_M "m")

set(JEVOIS_PLATFORM_BOOST_PYTHON "boost_python${JEVOIS_PLATFORM_PYTHON_MAJOR}${JEVOIS_PLATFORM_PYTHON_MINOR}")

########################################################################################################################
# Compiler flags optimized for platform:
set(JEVOIS_PLATFORM_ARCH_FLAGS "-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -ftree-vectorize -Ofast -funsafe-math-optimizations -mfp16-format=ieee")

# FIXME: libtbb uses some deprecated declarations:
set(JEVOIS_PLATFORM_ARCH_FLAGS "${JEVOIS_PLATFORM_ARCH_FLAGS} -Wno-deprecated-declarations")

########################################################################################################################
# Check for optional JEVOIS_SDK_ROOT environment variable, or use /usr/share/jevois-sdk by default:
if (DEFINED ENV{JEVOIS_SDK_ROOT})
  set(JEVOIS_SDK_ROOT $ENV{JEVOIS_SDK_ROOT})
else()
  set(JEVOIS_SDK_ROOT "/usr/share/${JEVOIS}-sdk")
endif()
message(STATUS "JeVois SDK root: ${JEVOIS_SDK_ROOT}")

# Locate buildroot base so we can use the compilers provided there to cross-compile for the platform:
set(JEVOIS_BUILD_BASE "${JEVOIS_SDK_ROOT}/out/sun8iw5p1/linux/common/buildroot")
message(STATUS "JeVois platform sysroot: ${JEVOIS_BUILD_BASE}")

########################################################################################################################
# Setup compilers to use on host:
set(JEVOIS_HOST_C_COMPILER "gcc-${JEVOIS_COMPILER_VERSION}")
set(JEVOIS_HOST_CXX_COMPILER "g++-${JEVOIS_COMPILER_VERSION}")
set(JEVOIS_HOST_FORTRAN_COMPILER "gfortran-${JEVOIS_COMPILER_VERSION}")

########################################################################################################################
# Setup the cross-compilers for the platform:
set(CROSS_COMPILE "${JEVOIS_BUILD_BASE}/host/usr/bin/arm-buildroot-linux-gnueabihf-")
set(JEVOIS_PLATFORM_C_COMPILER "${CROSS_COMPILE}gcc")
set(JEVOIS_PLATFORM_CXX_COMPILER "${CROSS_COMPILE}g++")
set(JEVOIS_PLATFORM_FORTRAN_COMPILER "${CROSS_COMPILE}gfortran")

########################################################################################################################
# OpenCV and other libraries on host and platform:

# Note: On host, get opencv from jevois-opencv package, in /usr/share/jevois-opencv-x.x.x; on platform it is in /usr
set(OPENCV_LIBS_FOR_JEVOIS "-lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_ml \
-lopencv_objdetect -lopencv_imgcodecs -lopencv_tracking -lopencv_video -lopencv_videoio -lopencv_dnn_objdetect \
-lopencv_dnn")

set(JEVOIS_PLATFORM_OPENCV_PREFIX "/usr")
set(JEVOIS_PLATFORM_OPENCV_LIBS "${OPENCV_LIBS_FOR_JEVOIS}") # note: do not use the prefix here since cross-compiling
set(JEVOIS_PLATFORM_NATIVE_OPENCV_LIBS "${OPENCV_LIBS_FOR_JEVOIS}")

set(JEVOIS_HOST_OPENCV_PREFIX "/usr/share/jevois-opencv-${JEVOIS_OPENCV_VERSION}")
set(JEVOIS_HOST_OPENCV_LIBS "-L${JEVOIS_HOST_OPENCV_PREFIX}/lib ${OPENCV_LIBS_FOR_JEVOIS} -lopencv_highgui")

# Use TBB and kernel includes for platform from the buildroot installation.  On the host, we may have local packages,
# eg, latest opencv compiled from source:
set(JEVOIS_PLATFORM_KERNEL_INCLUDE "-I${JEVOIS_BUILD_BASE}/build/linux-headers-3.4.113/usr/include")
set(JEVOIS_PLATFORM_TBB_INCLUDE "-I${JEVOIS_BUILD_BASE}/build/opencv3-${JEVOIS_OPENCV_VERSION}/buildroot-build/3rdparty/tbb/oneTBB-2020.2/include")

# Find python 3.x on host and platform:
# NOTE: it is too early here to try to use standard find_package() of CMake. In any case, that will not find the
# platform version which has to be done by hand here.
set(JEVOIS_PLATFORM_PYTHON_INCLUDE
  "-I${JEVOIS_BUILD_BASE}/host/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include/python${JEVOIS_PLATFORM_PYTHON_MAJOR}.${JEVOIS_PLATFORM_PYTHON_MINOR}${JEVOIS_PLATFORM_PYTHON_M} -I${JEVOIS_BUILD_BASE}/target/usr/lib/python${JEVOIS_PLATFORM_PYTHON_MAJOR}.${JEVOIS_PLATFORM_PYTHON_MINOR}/site-packages/numpy/core/include/")
set(JEVOIS_PLATFORM_PYTHON_LIBS "-lpython${JEVOIS_PLATFORM_PYTHON_MAJOR}.${JEVOIS_PLATFORM_PYTHON_MINOR}${JEVOIS_PLATFORM_PYTHON_M} -l${JEVOIS_PLATFORM_BOOST_PYTHON}")

set(JEVOIS_HOST_PYTHON_INCLUDE
  "-I/usr/include/python${JEVOIS_HOST_PYTHON_MAJOR}.${JEVOIS_HOST_PYTHON_MINOR}${JEVOIS_HOST_PYTHON_M}")
set(JEVOIS_HOST_PYTHON_LIBS "-lpython${JEVOIS_HOST_PYTHON_MAJOR}.${JEVOIS_HOST_PYTHON_MINOR}${JEVOIS_HOST_PYTHON_M} -l${JEVOIS_HOST_BOOST_PYTHON}")

# We link against OpenGL on JeVois-A33:
set(JEVOIS_HOST_OPENGL_LIBS "-lGLESv2 -lEGL -lX11 -lXext")
set(JEVOIS_PLATFORM_OPENGL_LIBS "-lGLESv2 -lEGL")

########################################################################################################################
# Include path:
set(JEVOIS_PLATFORM_INCLUDE "-I${JEVOIS_BUILD_BASE}/host/arm-buildroot-linux-gnueabihf/sysroot/usr/include/opencv4 ${JEVOIS_PLATFORM_KERNEL_INCLUDE} ${JEVOIS_PLATFORM_TBB_INCLUDE} ${JEVOIS_PLATFORM_PYTHON_INCLUDE}")
set(JEVOIS_HOST_INCLUDE "-I${JEVOIS_HOST_OPENCV_PREFIX}/include/opencv4 ${JEVOIS_HOST_PYTHON_INCLUDE}")




# FIXME: on GCC8, still need to link tolibstdc++fs to use recursive_directory_iretator:
set(JEVOIS_PLATFORM_PYTHON_LIBS "${JEVOIS_PLATFORM_PYTHON_LIBS} -lstdc++fs")

