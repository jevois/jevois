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

# CMake build rules for JeVois core library and executables

cmake_minimum_required(VERSION 3.1)

# Set vendor names, our modules will be placed in a directory by that name under /jevois/modules:
set(JEVOIS_VENDOR "JeVois")

# Project version:
set(JEVOIS_VERSION_MAJOR  1)
set(JEVOIS_VERSION_MINOR  0)
set(JEVOIS_SOVERSION "${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}" )

# Check for JEVOIS_SRC_ROOT environment variable:
if (DEFINED ENV{JEVOIS_SRC_ROOT})
  set(JEVOIS_SRC_ROOT $ENV{JEVOIS_SRC_ROOT})
else (DEFINED ENV{JEVOIS_SRC_ROOT})
  set(JEVOIS_SRC_ROOT "$ENV{HOME}/jevois")
  message(WARNING "You should set JEVOIS_SRC_ROOT environment variable to the root of your jevois source tree, needed \
to find the linux kernel, buildroot, cross-compilers, etc to install on the platform hardware. Using default: \
${JEVOIS_SRC_ROOT}")
endif (DEFINED ENV{JEVOIS_SRC_ROOT})

# Locate buildroot base so we can use the compilers provided there to cross-compile for the platform:
set(JEVOIS_BUILDROOT_BASE "${JEVOIS_SRC_ROOT}/../jevois-sdk/out/sun8iw5p1/linux/common/buildroot")

# Compilation options related to debugging:
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

# Check for JEVOIS_ROOT environment variable:
if (DEFINED ENV{JEVOIS_ROOT})
  set(JEVOIS_ROOT $ENV{JEVOIS_ROOT})
else (DEFINED ENV{JEVOIS_ROOT})
  set(JEVOIS_ROOT "/jevois")
  message(WARNING "You should set JEVOIS_ROOT environment variable to the root of where JeVois modules and "
    "config files will be installed. Using default: ${JEVOIS_ROOT}")
endif (DEFINED ENV{JEVOIS_ROOT})

# First define all vars for both host and platform here so we can patch jevois_config.cmake.in with both sets. Then,
# below, we will set the variables we need now to compile depending on whether JEVOIS_PLATFORM is set or not:

# On the host, install to /usr/local; on platform, install into buildroot's target directory:
set(JEVOIS_HOST_INSTALL_PREFIX "/usr/local")
set(JEVOIS_PLATFORM_INSTALL_PREFIX "${JEVOIS_BUILDROOT_BASE}/target/usr")

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

# Setup the cross-compilers for the platform:
set(CROSS_COMPILE "${JEVOIS_BUILDROOT_BASE}/host/usr/bin/arm-buildroot-linux-gnueabihf-")
set(JEVOIS_PLATFORM_C_COMPILER "${CROSS_COMPILE}gcc")
set(JEVOIS_PLATFORM_CXX_COMPILER "${CROSS_COMPILE}g++")

# Config for opencv libs.  note: videoio is for video capture, movies, etc and we may want to not use it on platform if
# we define it out in VideoInput.  In opencv3.1, imread() is in libopencv_imgcodecs.  On the host, we also link against
# opencv highgui to enable display windows, and prefer libs in /usr/local/lib:
set(JEVOIS_PLATFORM_OPENCV_LIBS "-lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_ml \
-lopencv_objdetect -lopencv_imgcodecs -lopencv_tracking -lopencv_video -lopencv_videoio")
set(JEVOIS_HOST_OPENCV_LIBS "-L${JEVOIS_HOST_INSTALL_PREFIX}/lib ${JEVOIS_PLATFORM_OPENCV_LIBS} -lopencv_highgui")

# Use TBB and kernel includes for platform from the buildroot installation.  On the host, we may have local packages,
# eg, latest opencv compiled from source:
set(JEVOIS_KERNEL_INCLUDE "-I${JEVOIS_BUILDROOT_BASE}/build/linux-headers-3.4.110/usr/include")
set(JEVOIS_TBB_INCLUDE "-I${JEVOIS_BUILDROOT_BASE}/build/opencv3-3.2.0/3rdparty/tbb/tbb44_20160128oss/include")
set(JEVOIS_PLATFORM_INCLUDE "${JEVOIS_KERNEL_INCLUDE} ${JEVOIS_TBB_INCLUDE}")
set(JEVOIS_HOST_INCLUDE "-I${JEVOIS_HOST_INSTALL_PREFIX}/include")

# On host, set JEVOIS_ROOT path as is for modules; on platform, add the buildroot path to JEVOIS_ROOT:
file(TO_NATIVE_PATH "${JEVOIS_ROOT}" JEVOIS_HOST_MODULES_ROOT)
file(TO_NATIVE_PATH "${JEVOIS_BUILDROOT_BASE}/target/${JEVOIS_ROOT}" JEVOIS_PLATFORM_MODULES_ROOT)

# Set compiler flags: FIXME: libtbb uses some deprecated declarations:
set(JEVOIS_HOST_CFLAGS "${JEVOIS_HOST_ARCH_FLAGS} -W -Wall -Wextra -g -O4 -I${JEVOIS_SRC_ROOT}/include \
${JEVOIS_HOST_INCLUDE} -fPIC -Wno-deprecated-declarations")
set(JEVOIS_PLATFORM_CFLAGS "${JEVOIS_PLATFORM_ARCH_FLAGS} -W -Wall -Wextra -g -O4 -I${JEVOIS_SRC_ROOT}/include \
${JEVOIS_PLATFORM_INCLUDE} -fPIC -Wno-deprecated-declarations")

# Include our helper functions, this will allow selection of host or platform and set variables accordingly:
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(JeVois)

# Project name, detects compiler (which has been set by our helper module):
project(jevois)

# Register dependencies on header files:
include_directories(include)

# Create our C++ config file:
configure_file(include/jevois/Config/Config.H.in Config.H @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.H DESTINATION include/jevois/Config COMPONENT bin)

# Pass the compiler flags to cmake (doing this before project() gives problems); same with th einstall prefix:
set(CMAKE_C_FLAGS "-std=gnu99 ${JEVOIS_CFLAGS} -include Config.H")
set(CMAKE_CXX_FLAGS "-std=c++17 ${JEVOIS_CFLAGS} -include Config.H")
set(CMAKE_INSTALL_PREFIX ${JEVOIS_INSTALL_PREFIX})

# Create our cmake config file for later compilation of modules and extra libs:
configure_file(src/jevois_config.cmake.in jevois_config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jevois_config.cmake
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

# Setup packaging:
set(CPACK_DEBIAN_PACKAGE_DEPENDS "build-essential, cmake")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, g++-5.2 (>=5.2)")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libtbb2")

# Setup our library:
file(GLOB_RECURSE JEVOIS_LIB_SRC_FILES src/jevois/*.C src/jevois/*.c)
add_library(jevois SHARED ${JEVOIS_LIB_SRC_FILES})
set_target_properties(jevois PROPERTIES VERSION "${JEVOIS_SOVERSION}" SOVERSION ${JEVOIS_SOVERSION})
link_libraries(jevois)
install(TARGETS jevois LIBRARY DESTINATION lib COMPONENT libs)


#---------------------------------------------------------------------
# OpenCV 2.2
#---------------------------------------------------------------------
#find_package(OpenCV2 QUIET)
#if(OpenCV2_FOUND)
#  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libopencv-dev, python-opencv")
#endif(OpenCV2_FOUND)
#NRT_LIBSTATUS(OpenCV2_FOUND "OpenCV2" INCLUDES ${OpenCV2_INCLUDE_DIRS} LIBRARIES ${OpenCV2_LIBRARIES}
#  EXTRAMESSAGE "try this: apt-get install libopencv-dev, python-opencv")
#set(NRT_HAVE_OPENCV2 ${OpenCV2_FOUND})

# Add the include/ directory to the compiler's search path
#include_directories(include)

# Add the tests directory, and tell the build system that we haveregression tests in there
#add_subdirectory(tests)
#enable_testing()

# Setup our executables:

#set(CMAKE_EXE_LINKER_FLAGS -Wl,-E)
set(JEVOIS_APP_LIBS
  "${JEVOIS_OPENCV_LIBS} -lpthread -ltbb -ldl -lboost_system -lboost_thread -lboost_regex -lturbojpeg")

add_executable(jevois-daemon src/Apps/jevois-daemon.C)
target_link_libraries(jevois-daemon jevois ${JEVOIS_APP_LIBS})
install(TARGETS jevois-daemon RUNTIME DESTINATION bin COMPONENT bin)

add_executable(jevois-module-param src/Apps/jevois-module-param.C)
target_link_libraries(jevois-module-param jevois ${JEVOIS_APP_LIBS})
install(TARGETS jevois-module-param RUNTIME DESTINATION bin COMPONENT bin)

add_executable(jevois-camtest src/Apps/jevois-camtest.C)
target_link_libraries(jevois-camtest jevois ${JEVOIS_APP_LIBS})
install(TARGETS jevois-camtest RUNTIME DESTINATION bin COMPONENT bin)

if (JEVOIS_PLATFORM)
  # On platform only, install jevois.sh from bin/ in the source tree into /usr/bin:
  install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/bin/jevois.sh" DESTINATION bin COMPONENT bin)
endif (JEVOIS_PLATFORM)

# Install all of our CMakeModules as well
file(GLOB JEVOIS_CMAKE_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/*.cmake)
install(FILES ${JEVOIS_CMAKE_MODULES} DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

# Custom targets to create the documentation:
add_custom_target(docsnip
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/extract-code-snippets.pl"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(doc
  COMMAND doxygen "${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen.cfg"
  DEPENDS docsnip
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# This is to install the doc of jevois and jevoisbase to jevois.org, only works in ilab:
add_custom_target(docweb
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/docinstall.sh
  DEPENDS doc
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Configure our pkgconfig file:
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/jevois.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/jevois.pc @ONLY)

# Documentation files for our install / distribution package
set(DOC_FILES README INSTALL COPYING)
set(DOC_PATH "share/doc/jevois")
install(FILES ${DOC_FILES} DESTINATION ${DOC_PATH} COMPONENT bin)

# add includes to our install / distribution package (see the CMakeLists.txt in these directories)
add_subdirectory(include)

# Add installation rules for files that go into /jevois:
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/videomappings.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/params.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Config/initscript.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
#install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
#  USE_SOURCE_PERMISSIONS
#  DESTINATION "${JEVOIS_MODULES_ROOT}" COMPONENT bin)

# Figure out subversion version, to use during packaging:
#find_package(Subversion QUIET)
#if (Subversion_FOUND)
#  Subversion_WC_INFO(${CMAKE_CURRENT_SOURCE_DIR} JEVOIS)
#  set(SUBVERSION_REVISION ${JEVOIS_WC_REVISION})
#else (Subversion_FOUND)
set(SUBVERSION_REVISION "unknown")
#endif (Subversion_FOUND)

# Create packages (Debian, RPM): in build/, just type 'cpack' to create the package.
# To list the files created in a package, run: dpkg --contents <package.deb>
set(CPACK_PACKAGE_DESCRIPTION "JeVois Embedded Machine Vision")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "JeVois Embedded Machine Vision from iLab at the University of Southern California")
set(CPACK_PACKAGE_NAME "jevois")

set(CPACK_PACKAGE_CONTACT "Laurent Itti")
set(CPACK_PACKAGE_VENDOR "iLab at the University of Southern California")
set(CPACK_PACKAGE_VERSION_MAJOR "${JEVOIS_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${JEVOIS_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${SUBVERSION_REVISION}")
set(VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/scripts/postinst;${CMAKE_SOURCE_DIR}/scripts/prerm;")

set(CPACK_GENERATOR "DEB;")  # could be DEB;RPM;
set(CPACK_PACKAGE_FILE_NAME
  "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-${CMAKE_SYSTEM_PROCESSOR}")

SET(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}/*")

#set(CPACK_DEB_COMPONENT_INSTALL ON)
#set(CPACK_COMPONENTS_ALL libs bin)
include(CPack)
