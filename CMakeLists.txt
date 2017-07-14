######################################################################################################################
#
# JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
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

# CMAKE BUILD RULES FOR JEVOIS CORE LIBRARY AND EXECUTABLES

CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

########################################################################################################################
# Set vendor names, our modules will be placed in a directory by that name under /jevois/modules:
set(JEVOIS_VENDOR "JeVois")

########################################################################################################################
# Project version:
set(JEVOIS_VERSION_MAJOR  1)
set(JEVOIS_VERSION_MINOR  3)
set(JEVOIS_VERSION_PATCH  1)
set(JEVOIS_SOVERSION "${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}.${JEVOIS_VERSION_PATCH}" )

########################################################################################################################
# Python version to use for python modules:
set(JEVOIS_PYTHON_MAJOR 3)
set(JEVOIS_PYTHON_MINOR 5)
set(JEVOIS_PYTHON_M "m")

########################################################################################################################
# OpenCV version to use with JeVois:
set(JEVOIS_OPENCV_MAJOR 3)
set(JEVOIS_OPENCV_MINOR 2)
set(JEVOIS_OPENCV_PATCH 0)
set(JEVOIS_OPENCV_VERSION "${JEVOIS_OPENCV_MAJOR}.${JEVOIS_OPENCV_MINOR}.${JEVOIS_OPENCV_PATCH}")

########################################################################################################################
# Check for optional JEVOIS_SDK_ROOT environment variable, or use /usr/share/jevois-sdk by default:
if (DEFINED ENV{JEVOIS_SDK_ROOT})
  set(JEVOIS_SDK_ROOT $ENV{JEVOIS_SDK_ROOT})
else (DEFINED ENV{JEVOIS_SDK_ROOT})
  set(JEVOIS_SDK_ROOT "/usr/share/jevois-sdk")
endif (DEFINED ENV{JEVOIS_SDK_ROOT})

# Locate buildroot base so we can use the compilers provided there to cross-compile for the platform:
set(JEVOIS_BUILDROOT_BASE "${JEVOIS_SDK_ROOT}/out/sun8iw5p1/linux/common/buildroot")

########################################################################################################################
# Compilation options that can be set by users:
option(JEVOIS_LDEBUG_ENABLE "Enable LDEBUG() messages. If turned off, LDEBUG() will not be not compiled in, and no \
message will be issued even if the log level is set to LOG_DEBUG at runtime. This is to avoid doing a lot of \
runtime tests on the log level to decide whether it is LOG_DEBUG or not." OFF)
message(STATUS "JEVOIS_LDEBUG_ENABLE: ${JEVOIS_LDEBUG_ENABLE}")

option(JEVOIS_TRACE_ENABLE "Enable tracing of functions that use JEVOIS_TRACE(). They will not be compiled in if \
OFF. Note that JEVOIS_TRACE uses LDEBUG() so JEVOIS_LDEBUG_ENABLE should be ON to see the trace messages" OFF)
message(STATUS "JEVOIS_TRACE_ENABLE: ${JEVOIS_TRACE_ENABLE}")

option(JEVOIS_USE_SYNC_LOG "Enable synchronous logging, i.e., log messages from LDEBUG(), LINFO(), etc are issued \
immediately and execution flow blocks until they are fully printed out. This may sometimes be too slow in \
fast streaming applications if the printing happens over a slow serial link. Hence, default behavior is to use \
an asynchronous queue for the messages, where LDEBUG(), LINFO(), etc just queue up the message string and a \
background thread prints them out of the queue as fast as it can. The order in which the messages were issued \
is preserved by the queue, but there may be delays between message issue and printing, hence JEVOIS_USE_SYNC_LOG \
may be useful in cases where one wants to check system messages (e.g., syslog printing on the console) and \
their timing with respect to user application messages" OFF)
message(STATUS "JEVOIS_USE_SYNC_LOG: ${JEVOIS_USE_SYNC_LOG}")

option(JEVOIS_LOG_TO_FILE "Enable sending all log messages to file jevois.log instead of console. Only works \
with async logging." OFF)
message(STATUS "JEVOIS_LOG_TO_FILE: ${JEVOIS_LOG_TO_FILE}")

option(HOST_IS_RPI3 "When compiling for host, assume a Raspberry Pi 3 instead of default Intel" OFF)
message(STATUS "HOST_IS_RPI3: ${HOST_IS_RPI3}")

########################################################################################################################
# Check for JEVOIS_ROOT environment variable:
if (DEFINED ENV{JEVOIS_ROOT})
  set(JEVOIS_ROOT $ENV{JEVOIS_ROOT})
else (DEFINED ENV{JEVOIS_ROOT})
  set(JEVOIS_ROOT "/jevois")
endif (DEFINED ENV{JEVOIS_ROOT})

########################################################################################################################
# First define all vars for both host and platform here so we can patch jevois_config.cmake.in with both sets. Then,
# below, we will set the variables we need now to compile depending on whether JEVOIS_PLATFORM is set or not:

# On the host, install to /usr ; on platform, install to /var/lib/jevois-build/usr
set(JEVOIS_HOST_INSTALL_PREFIX "/usr")
set(JEVOIS_PLATFORM_INSTALL_PREFIX "/var/lib/jevois-build/usr")

########################################################################################################################
# Architecture flags to optimize compilation:
if (HOST_IS_RPI3)
  # Should optimize for A53 on RPI3, but fails to compile TBB. /proc/cpuinfo on RPI3 reports ARMv7 and vfpv4 anyway
  #set(JEVOIS_HOST_ARCH_FLAGS "-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard -ftree-vectorize -Ofast")
  set(JEVOIS_HOST_ARCH_FLAGS "-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -ftree-vectorize -Ofast")
else (HOST_IS_RPI3)
  set(JEVOIS_HOST_ARCH_FLAGS "-msse4 -Ofast")
endif (HOST_IS_RPI3)

set(JEVOIS_PLATFORM_ARCH_FLAGS "-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -ftree-vectorize -Ofast")
# NOTE: could add -funsafe-math-optimizations to use NEON for general maths but NEON is not 100% IEEE-754 compatible

########################################################################################################################
# Setup the cross-compilers for the platform:
set(CROSS_COMPILE "${JEVOIS_BUILDROOT_BASE}/host/usr/bin/arm-buildroot-linux-gnueabihf-")
set(JEVOIS_PLATFORM_C_COMPILER "${CROSS_COMPILE}gcc")
set(JEVOIS_PLATFORM_CXX_COMPILER "${CROSS_COMPILE}g++")

########################################################################################################################
# Libraries and helpers needed to compile and run:

# Config for opencv libs.  note: videoio is for video capture, movies, etc and we may want to not use it on platform if
# we define it out in VideoInput.  In opencv3.1, imread() is in libopencv_imgcodecs.  On the host, we also link against
# opencv highgui to enable display windows.
# Note: On host, get opencv from jevois-opencv package, in /usr/share/jevois-opencv-3.2.0, on platform it is in /usr
set(OPENCV_LIBS_FOR_JEVOIS "-lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_ml \
-lopencv_objdetect -lopencv_imgcodecs -lopencv_tracking -lopencv_video -lopencv_videoio")

set(JEVOIS_PLATFORM_OPENCV_PREFIX "/usr")
set(JEVOIS_PLATFORM_OPENCV_LIBS "${OPENCV_LIBS_FOR_JEVOIS}") # note: do not use the prefix here since cross-compiling

set(JEVOIS_HOST_OPENCV_PREFIX "/usr/share/jevois-opencv-${JEVOIS_OPENCV_VERSION}")
set(JEVOIS_HOST_OPENCV_LIBS "-L${JEVOIS_HOST_OPENCV_PREFIX}/lib ${OPENCV_LIBS_FOR_JEVOIS} -lopencv_highgui")

# Use TBB and kernel includes for platform from the buildroot installation.  On the host, we may have local packages,
# eg, latest opencv compiled from source:
set(JEVOIS_KERNEL_INCLUDE "-I${JEVOIS_BUILDROOT_BASE}/build/linux-headers-3.4.110/usr/include")
set(JEVOIS_TBB_INCLUDE "-I${JEVOIS_BUILDROOT_BASE}/build/opencv3-3.2.0/3rdparty/tbb/tbb44_20160128oss/include")

# Find python 3.x on host and platform:
# NOTE: it is too early here to try to use standard find_package() of CMake. In any case, that will not find the
# platform version which has to be done by hand here.
set(JEVOIS_PLATFORM_PYTHON_INCLUDE
  "-I${JEVOIS_BUILDROOT_BASE}/host/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include/python${JEVOIS_PYTHON_MAJOR}.${JEVOIS_PYTHON_MINOR}${JEVOIS_PYTHON_M} -I${JEVOIS_BUILDROOT_BASE}/target/usr/lib/python${JEVOIS_PYTHON_MAJOR}.${JEVOIS_PYTHON_MINOR}/site-packages/numpy/core/include/")
set(JEVOIS_PLATFORM_PYTHON_LIBS "-lpython${JEVOIS_PYTHON_MAJOR}.${JEVOIS_PYTHON_MINOR}${JEVOIS_PYTHON_M} -lboost_python${JEVOIS_PYTHON_MAJOR}")
set(JEVOIS_HOST_PYTHON_INCLUDE
  "-I/usr/include/python${JEVOIS_PYTHON_MAJOR}.${JEVOIS_PYTHON_MINOR}${JEVOIS_PYTHON_M}")
set(JEVOIS_HOST_PYTHON_LIBS "-lpython${JEVOIS_PYTHON_MAJOR}.${JEVOIS_PYTHON_MINOR}${JEVOIS_PYTHON_M} -lboost_python-py${JEVOIS_PYTHON_MAJOR}${JEVOIS_PYTHON_MINOR}")
  
########################################################################################################################
# Include path:
set(JEVOIS_PLATFORM_INCLUDE "${JEVOIS_KERNEL_INCLUDE} ${JEVOIS_TBB_INCLUDE} ${JEVOIS_PLATFORM_PYTHON_INCLUDE}")
set(JEVOIS_HOST_INCLUDE "-I${JEVOIS_HOST_OPENCV_PREFIX}/include ${JEVOIS_HOST_PYTHON_INCLUDE}")

########################################################################################################################
# Root path to install modules:
# For host, set JEVOIS_ROOT path as is for modules; for platform, put it into /var/lib/jevois-microsd:
file(TO_NATIVE_PATH "${JEVOIS_ROOT}" JEVOIS_HOST_MODULES_ROOT)
file(TO_NATIVE_PATH "/var/lib/jevois-microsd" JEVOIS_PLATFORM_MODULES_ROOT)

########################################################################################################################
# Set compiler flags: FIXME: libtbb uses some deprecated declarations:
set(JEVOIS_HOST_CFLAGS "${JEVOIS_HOST_ARCH_FLAGS} -W -Wall -Wextra -g -O4 -I${JEVOIS_SRC_ROOT}/jevois/include \
${JEVOIS_HOST_INCLUDE} -fPIC -Wno-deprecated-declarations")
set(JEVOIS_PLATFORM_CFLAGS "${JEVOIS_PLATFORM_ARCH_FLAGS} -W -Wall -Wextra -g -O4 -I${JEVOIS_SRC_ROOT}/jevois/include \
${JEVOIS_PLATFORM_INCLUDE} -fPIC -Wno-deprecated-declarations")

########################################################################################################################
# Include our helper functions, this will allow selection of host or platform and set variables accordingly:
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(JeVois)

########################################################################################################################
# Project name, detects compiler (which has been set by our helper module) and general setup
project(jevois)

# Set the cmake install prefix:
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX ${JEVOIS_INSTALL_PREFIX} CACHE PATH "Installation prefix" FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Register dependencies on header files:
include_directories(include)

# Create our C++ config file:
configure_file(include/jevois/Config/Config.H.in Config.H @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.H DESTINATION include/jevois/Config COMPONENT bin)

# Pass the compiler flags to cmake (doing this before project() gives problems with wrong compiler detection):
set(CMAKE_C_FLAGS "-std=gnu99 ${JEVOIS_CFLAGS} -include Config.H")
set(CMAKE_CXX_FLAGS "-std=c++17 ${JEVOIS_CFLAGS} -include Config.H")

# Create our cmake config file for later compilation of modules and extra libs:
configure_file(src/jevois_config.cmake.in jevois_config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jevois_config.cmake
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

# Setup our library:
file(GLOB_RECURSE JEVOIS_LIB_SRC_FILES src/jevois/*.C src/jevois/*.c)
add_library(jevois SHARED ${JEVOIS_LIB_SRC_FILES})
set_target_properties(jevois PROPERTIES VERSION "${JEVOIS_SOVERSION}" SOVERSION ${JEVOIS_SOVERSION})
link_libraries(jevois)
install(TARGETS jevois LIBRARY DESTINATION lib COMPONENT libs)
target_link_libraries(jevois ${JEVOIS_OPENCV_LIBS} -lpthread -ltbb -ldl -lutil -lboost_system
  -lboost_thread -lboost_regex -lturbojpeg -lm ${JEVOIS_PYTHON_LIBS})

# Setup our executables:
add_executable(jevois-daemon src/Apps/jevois-daemon.C)
target_link_libraries(jevois-daemon jevois)
install(TARGETS jevois-daemon RUNTIME DESTINATION bin COMPONENT bin)

add_executable(jevois-module-param src/Apps/jevois-module-param.C)
target_link_libraries(jevois-module-param jevois)
install(TARGETS jevois-module-param RUNTIME DESTINATION bin COMPONENT bin)

add_executable(jevois-camtest src/Apps/jevois-camtest.C)
target_link_libraries(jevois-camtest jevois)
install(TARGETS jevois-camtest RUNTIME DESTINATION bin COMPONENT bin)

add_executable(jevois-add-videomapping src/Apps/jevois-add-videomapping.C)
target_link_libraries(jevois-add-videomapping jevois)
install(TARGETS jevois-add-videomapping RUNTIME DESTINATION bin COMPONENT bin)

if (JEVOIS_PLATFORM)
  # On platform only, install jevois.sh from bin/ in the source tree into /usr/bin:
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/bin/jevois.sh" DESTINATION bin COMPONENT bin)
else (JEVOIS_PLATFORM)
  # On host only, install helper scripts:
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jevois-jvpkg" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jevois-create-module" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jevois-create-python-module" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jevois-modinfo" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jevois-usbsd" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jevois-cmd" DESTINATION bin COMPONENT bin)
endif (JEVOIS_PLATFORM)

# Install all of our CMakeModules as well
file(GLOB JEVOIS_CMAKE_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/*.cmake)
install(FILES ${JEVOIS_CMAKE_MODULES} DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

########################################################################################################################
# Documentation:

# Extract code snippets: FIXME could revisit using \snip tags instead
add_custom_target(docsnip
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/extract-code-snippets.pl"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(doc
  COMMAND doxygen "${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen.cfg"
  DEPENDS docsnip
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# This is to install the doc of jevois and jevoisbase to jevois.org, only works in ilab:
if ($ENV{JEVOIS_ILAB})
  add_custom_target(docweb
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docinstall.sh
    DEPENDS doc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ($ENV{JEVOIS_ILAB})

# Configure our pkgconfig file:
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/jevois.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/jevois.pc @ONLY)

# Documentation files for our install / distribution package
set(DOC_FILES README INSTALL COPYING)
if (JEVOIS_PLATFORM)
  set(DOC_PATH "share/doc/jevois-platform")
else (JEVOIS_PLATFORM)
  set(DOC_PATH "share/doc/jevois-host")
endif (JEVOIS_PLATFORM)
install(FILES ${DOC_FILES} DESTINATION ${DOC_PATH} COMPONENT bin)

########################################################################################################################
# Extra files to be installed:

# Add includes to our install / distribution package:
add_subdirectory(include)

# Add installation rules for files that go into /jevois:
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/videomappings.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/params.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/initscript.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

########################################################################################################################
# Debian packaging:

# Create packages (Debian, RPM): in hbuild/ or pbuild/, just type 'sudo cpack' to create the package.
# To list the files created in a package, run: dpkg --contents <package.deb>
set(CPACK_PACKAGE_DESCRIPTION "JeVois Smart Machine Vision Core")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JeVois Smart Embedded Machine Vision Toolkit, Core")
set(CPACK_PACKAGE_CONTACT "Laurent Itti <jevois.org@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_DEBIAN_PACKAGE_SECTION "universe")
set(CPACK_PACKAGE_VENDOR "iLab at the University of Southern California")
set(CPACK_PACKAGE_VERSION_MAJOR "${JEVOIS_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${JEVOIS_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${JEVOIS_VERSION_PATCH}")
set(JEVOIS_PACKAGE_RELEASE "1") # packager revision number

set(CPACK_DEBIAN_PACKAGE_DEPENDS "build-essential, cmake (>= 3.1), gcc (>= 5.2), g++ (>=5.2), gfortran (>= 5.2), \
guvcview, subversion, cmake, git, mercurial, doxygen-gui, graphviz, libboost-all-dev, libjpeg-turbo8-dev, autoconf, \
libeigen3-dev, screen, libgtk2.0-dev, libdc1394-22, libdc1394-22-dev, libjpeg-dev, libpng-dev, libtiff5-dev, \
libavcodec-dev, libavformat-dev, libswscale-dev, libxine2-dev, libgstreamer1.0-dev, libgstreamer-plugins-base1.0-dev, \
libv4l-dev, libtbb2, libtbb-dev, libqt4-dev, libfaac-dev, libmp3lame-dev, libopencore-amrnb-dev, \
libopencore-amrwb-dev, libtheora-dev, libvorbis-dev, libxvidcore-dev, x264, v4l-utils, unzip, qt5-default, \
python3-dev, python3-numpy, python3-pip, libgtk-3-dev, libatlas-base-dev, libturbojpeg, libtbb2, \
python${JEVOIS_PYTHON_MAJOR}.${JEVOIS_PYTHON_MINOR}-dev, python${JEVOIS_PYTHON_MAJOR}-numpy, \
jevois-opencv (>=${JEVOIS_OPENCV_VERSION})")

# Use helper from JeVois.cmake for all other settings:
jevois_setup_cpack("jevois")
