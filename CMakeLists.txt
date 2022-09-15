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
set(JEVOIS_VERSION_MINOR  18)
set(JEVOIS_VERSION_PATCH  0)
set(JEVOIS_SOVERSION "${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}.${JEVOIS_VERSION_PATCH}" )


########################################################################################################################
# OpenCV version to use with JeVois:
set(JEVOIS_OPENCV_MAJOR 4)
set(JEVOIS_OPENCV_MINOR 6)
set(JEVOIS_OPENCV_PATCH 0)
set(JEVOIS_OPENCV_VERSION "${JEVOIS_OPENCV_MAJOR}.${JEVOIS_OPENCV_MINOR}.${JEVOIS_OPENCV_PATCH}")

set(JEVOIS_OPENVINO_VERSION "2022.1")

message(STATUS "JeVois OpenCV: ${JEVOIS_OPENCV_VERSION}")
message(STATUS "JeVois OpenVino: ${JEVOIS_OPENVINO_VERSION}")

########################################################################################################################
# Select target hardware amd include A33/pro specific definitions.
# The following variables get defined accordingly for use below and in C++ code:
# JEVOIS_A33 - for the original JeVois hardware using Allwinner A33 processor. Files are installed in
#              /jevois, /var/lib/jevois-microsd, etc. JEVOIS is set to "jevois".
# JEVOIS_PRO - for the pro hardware using Amlogic A311D processor. Files are installed in
#              /jevoispro, /var/lib/jevoispro-microsd, etc. JEVOIS is set to "jevoispro".
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(JeVoisHardware)

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

########################################################################################################################
# Detect whether host processor is ARM, so we can set some compiler flags (use NEON instead of SSE, etc):
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
  set(HOST_IS_ARM ON)
else ()
  set(HOST_IS_ARM OFF)
endif ()
  
message(STATUS "HOST_IS_ARM: ${HOST_IS_ARM}")

########################################################################################################################
# Check for JEVOIS_ROOT environment variable:
if (DEFINED ENV{JEVOIS_ROOT})
  set(JEVOIS_ROOT $ENV{JEVOIS_ROOT})
else ()
  set(JEVOIS_ROOT "/${JEVOIS}")
endif ()

########################################################################################################################
# First define all vars for both host and platform here so we can patch jevois_config.cmake.in with both sets. Then,
# below, we will set the variables we need now to compile depending on whether JEVOIS_PLATFORM is set or not:

# On the host, install to /usr ; on platform, install to /var/lib/jevois[pro]-build/usr
set(JEVOIS_HOST_INSTALL_PREFIX "/usr")
set(JEVOIS_PLATFORM_INSTALL_PREFIX "/var/lib/${JEVOIS}-build/usr")
set(JEVOIS_PLATFORM_INSTALL_PREFIX_PDEB "/var/lib/${JEVOIS}-build-pdeb/usr")

########################################################################################################################
# Architecture flags to optimize compilation (platform flags defines in JeVoisA33.cmake, JeVoisPro.cmake, etc):
if (HOST_IS_ARM)
  set(JEVOIS_HOST_ARCH_FLAGS "${JEVOIS_PLATFORM_ARCH_FLAGS}")
else (HOST_IS_ARM)
  set(JEVOIS_HOST_ARCH_FLAGS "-msse4 -Ofast")
endif (HOST_IS_ARM)

########################################################################################################################
# Root path to install modules:
# For host, set JEVOIS_ROOT path as is for modules; for platform, put it into /var/lib/jevois-microsd:
file(TO_NATIVE_PATH "${JEVOIS_ROOT}" JEVOIS_HOST_MODULES_ROOT)
file(TO_NATIVE_PATH "/var/lib/${JEVOIS}-microsd" JEVOIS_PLATFORM_MODULES_ROOT)
file(TO_NATIVE_PATH "/var/lib/${JEVOIS}-build-pdeb" JEVOIS_PLATFORM_MODULES_ROOT_PDEB)

########################################################################################################################
# Set compiler flags:
set(JEVOIS_HOST_CFLAGS "${JEVOIS_HOST_ARCH_FLAGS} -W -Wall -Wextra -g -O4 -I${JEVOIS_SRC_ROOT}/jevois/include \
${JEVOIS_HOST_INCLUDE} -fPIC -Wno-deprecated-declarations")
set(JEVOIS_PLATFORM_CFLAGS "${JEVOIS_PLATFORM_ARCH_FLAGS} -W -Wall -Wextra -g -O4 -I${JEVOIS_SRC_ROOT}/jevois/include \
${JEVOIS_PLATFORM_INCLUDE} -fPIC -Wno-psabi")

########################################################################################################################
# Include our helper functions, this will allow selection of host or platform and set variables accordingly:
include(JeVois)

# Check that we are not trying to compile a newer jevois against an older jevois-sdk, which would typically fail:
jevois_check_sdk_version(${JEVOIS_SOVERSION})

########################################################################################################################
# Project name, detects compiler (which has been set by our helper module) and general setup
project(${JEVOIS})

# Set the cmake install prefix:
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX ${JEVOIS_INSTALL_PREFIX} CACHE PATH "Installation prefix" FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Register dependencies on header files:
include_directories(include)

# Create our C++ config file:
configure_file(include/jevois/Config/Config.H.in Config-${JEVOIS}.H @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Config-${JEVOIS}.H DESTINATION include/jevois/Config COMPONENT bin)

# Create our Python config file:
configure_file(src/pyjevois.py.in pyjevois.py @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyjevois.py DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

# Pass the compiler flags to cmake (doing this before project() gives problems with wrong compiler detection):
set(CMAKE_C_FLAGS "-std=gnu99 ${JEVOIS_CFLAGS} -include Config-${JEVOIS}.H")
if (JEVOIS_PRO)
  set(CMAKE_CXX_FLAGS "-std=c++20 ${JEVOIS_CFLAGS} -include Config-${JEVOIS}.H") # g++-10 supports c++-20
else()
  set(CMAKE_CXX_FLAGS "-std=c++2a ${JEVOIS_CFLAGS} -include Config-${JEVOIS}.H") # g++-8.3 supports c++-2a
endif()

# Enable address sanitizer (to debug illegal memory accesses and such; should normally be commented out):
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

# Create our cmake config file for later compilation of modules and extra libs:
configure_file(src/jevois_config.cmake.in jevois_config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jevois_config.cmake DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)

# Setup our library:
file(GLOB_RECURSE JEVOIS_LIB_SRC_FILES src/jevois/*.C src/jevois/*.c)
add_library(${JEVOIS} SHARED ${JEVOIS_LIB_SRC_FILES})
set_target_properties(${JEVOIS} PROPERTIES VERSION "${JEVOIS_SOVERSION}" SOVERSION ${JEVOIS_SOVERSION})
link_libraries(${JEVOIS})
install(TARGETS ${JEVOIS} LIBRARY DESTINATION lib COMPONENT libs)
target_link_libraries(${JEVOIS} ${JEVOIS_OPENCV_LIBS} -lpthread -ltbb -ldl -lutil -lboost_system
  -lboost_thread -lboost_regex -lturbojpeg -lm ${JEVOIS_PYTHON_LIBS} ${JEVOIS_OPENGL_LIBS})

########################################################################################################################
# Thread pool support:
include_directories(Contrib/threadpool/libs/function2/include)
include_directories(Contrib/threadpool/libs/concurrentqueue)

########################################################################################################################
# Setup imgui on JeVois-Pro:
if (JEVOIS_PRO)
  include_directories(Contrib/imgui Contrib/imgui/backends Contrib)
  target_sources(${JEVOIS} PRIVATE Contrib/imgui/imgui.cpp
    Contrib/imgui/imgui_draw.cpp
    Contrib/imgui/imgui_widgets.cpp
    Contrib/imgui/backends/imgui_impl_opengl3.cpp
    Contrib/imgui/imgui_demo.cpp
    Contrib/imgui/imgui_tables.cpp
    Contrib/ImGuiColorTextEdit/TextEditor.cpp)

  if (JEVOIS_PLATFORM)
    # nothing extra here
  else ()
    target_sources(${JEVOIS} PRIVATE Contrib/imgui/backends/imgui_impl_sdl.cpp)
  endif()

  # Flags for imgui:
  add_definitions(-DIMGUI_IMPL_OPENGL_ES3 -DIMGUI_IMPL_OPENGL_ES2 -D_REENTRANT)
endif ()

########################################################################################################################
# NPU libraries for JeVois-Pro A311D processor:

# Note: We include the NPU includes on all architectures, so that we can use the nice attr struct of the NPU to describe
# input and output tensor encoding and quantization:
include_directories(Contrib/npu/include/ovxlib)
include_directories(Contrib/npu/include/ovxlib/utils)
include_directories(Contrib/npu/include/ovxlib/client)
include_directories(Contrib/npu/include/ovxlib/ops)
include_directories(Contrib/npu/include)
include_directories(Contrib/npu/detect_library/source_code/include)

if (JEVOIS_PRO)
  if (JEVOIS_PLATFORM)
    add_definitions(-DLINUX -D_REENTRANT -DgcdENABLE_3D=1 -DgcdENABLE_2D=0 -DgcdENABLE_VG=0 -DgcdUSE_VX=1 -DUSE_VDK=1
      -DgcdMOVG=0 -DEGL_API_FB -DgcdSTATIC_LINK=0 -DgcdFPGA_BUILD=0 -DGC_ENABLE_LOADTIME_OPT=1 -DgcdUSE_VXC_BINARY=0
      -DgcdGC355_MEM_PRINT=0 -DgcdGC355_PROFILER=0 -DVIVANTE_PROFILER=0 -DVIVANTE_PROFILER_CONTEXT=0)

    # Link libraries. These are all pre-installed in jevoispro-sysroot:
    target_link_libraries(${JEVOIS} OpenVX OpenVXU CLC VSC GAL ovxlib)
  else ()
    # Link libraries. These are all pre-installed in jevois/Contrib and upated when OpenCV is built:
    target_link_libraries(${JEVOIS} -L${CMAKE_CURRENT_SOURCE_DIR}/Contrib/npu/x86_64_linux/lib
      OpenVX OpenVXU OpenVXC CLC VSC GAL Emulator ArchModelSw NNArchPerf vdtproxy)
  endif()
  
endif()

########################################################################################################################
# Hailo neural accelerator libraries
if (JEVOIS_PRO)
  # Link libraries. These are all pre-installed in jevoispro-sysroot:
  target_link_libraries(${JEVOIS} hailort)
endif ()

########################################################################################################################
# Coral edge TPU libraries and associated TensorFlow. We install libtensorflowlite.so later as part of our package:
include_directories(Contrib/tensorflow)
include_directories(Contrib/tensorflow/tensorflow/lite/tools/make/downloads/flatbuffers/include)
target_link_libraries(${JEVOIS} tensorflowlite)

if (JEVOIS_PRO)
  # Coral edge TPU libraries for JeVois-Pro:
  target_link_libraries(${JEVOIS} edgetpu)
else()
  if (JEVOIS_PLATFORM)
    # Need to specify path for libtensorflowlite.so; not sure why it is not found in the target buildroot tree:
    target_link_libraries(${JEVOIS} -L${JEVOIS_PLATFORM_MODULES_ROOT}/lib)
  endif()
endif()

########################################################################################################################
# Custom command to pre-process our OpenGL shaders (only used by JeVois-Pro):
if (JEVOIS_PRO)
  set(JEVOIS_SHADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders.C")
  set(JEVOIS_SHADER_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/src/jevois/GPU/shaders")
  
  add_custom_command(OUTPUT ${JEVOIS_SHADER_FILE}
    COMMENT "Converting OpenGL shaders to C++ arrays"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/glsl-to-cpp.sh ${JEVOIS_SHADER_FILE} ${JEVOIS_SHADER_SRCDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${JEVOIS_SHADER_SRCDIR}/*.glsl )

  target_sources(${JEVOIS} PRIVATE ${JEVOIS_SHADER_FILE})
endif ()

########################################################################################################################
# Setup our executables:
add_executable(${JEVOIS}-daemon src/Apps/jevois-daemon.C)
target_link_libraries(${JEVOIS}-daemon ${JEVOIS})
install(TARGETS ${JEVOIS}-daemon RUNTIME DESTINATION bin COMPONENT bin)

add_executable(${JEVOIS}-module-param src/Apps/jevois-module-param.C)
target_link_libraries(${JEVOIS}-module-param ${JEVOIS})
install(TARGETS ${JEVOIS}-module-param RUNTIME DESTINATION bin COMPONENT bin)

add_executable(${JEVOIS}-camtest src/Apps/jevois-camtest.C)
target_link_libraries(${JEVOIS}-camtest ${JEVOIS})
install(TARGETS ${JEVOIS}-camtest RUNTIME DESTINATION bin COMPONENT bin)

add_executable(${JEVOIS}-add-videomapping src/Apps/jevois-add-videomapping.C)
target_link_libraries(${JEVOIS}-add-videomapping ${JEVOIS})
install(TARGETS ${JEVOIS}-add-videomapping RUNTIME DESTINATION bin COMPONENT bin)

if (JEVOIS_PRO)
  add_executable(${JEVOIS}-restore-console src/Apps/jevois-restore-console.C)
  target_link_libraries(${JEVOIS}-restore-console ${JEVOIS})
  install(TARGETS ${JEVOIS}-restore-console RUNTIME DESTINATION bin COMPONENT bin)
endif ()

if (JEVOIS_PLATFORM)
  # On platform only, install jevois[pro].sh from bin/ in the source tree into /usr/bin:
  install(PROGRAMS "bin/${JEVOIS}.sh" DESTINATION bin COMPONENT bin)
else (JEVOIS_PLATFORM)
  # On host only, install helper scripts:
  install(PROGRAMS "scripts/jevois-jvpkg" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "scripts/jevois-create-module" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "scripts/jevois-create-python-module" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "scripts/jevois-modinfo" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "scripts/jevois-usbsd" DESTINATION bin COMPONENT bin)
  install(PROGRAMS "scripts/jevois-cmd" DESTINATION bin COMPONENT bin)
endif (JEVOIS_PLATFORM)

# Install all of our CMakeModules as well
file(GLOB JEVOIS_CMAKE_MODULES CMakeModules/*.cmake)
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
  set(DOC_PATH "share/doc/${JEVOIS}-platform")
else (JEVOIS_PLATFORM)
  set(DOC_PATH "share/doc/${JEVOIS}-host")
endif (JEVOIS_PLATFORM)
install(FILES ${DOC_FILES} DESTINATION ${DOC_PATH} COMPONENT bin)

########################################################################################################################
# Extra files to be installed:

# Add includes to our install / distribution package:
add_subdirectory(include)

# Add installation rules for files that go into /jevois[pro]:
if (JEVOIS_PRO)
  install(FILES "Config/videomappings-Pro.cfg"
    DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin RENAME "videomappings.cfg")

  # Copy imgui header files to /jevois[pro]/include so jevoisbase and others can find them:
  file(GLOB IMGUIINCLFILES "Contrib/imgui/*.h")
  install(FILES ${IMGUIINCLFILES} DESTINATION "${JEVOIS_MODULES_ROOT}/include/imgui" COMPONENT bin)

  # Copy ImGuiColorTextEdit header files to /jevois[pro]/include so jevoisbase and others can find them:
  file(GLOB IMGUITXTINCLFILES "Contrib/ImGuiColorTextEdit/*.h")
  install(FILES ${IMGUITXTINCLFILES} DESTINATION "${JEVOIS_MODULES_ROOT}/include/ImGuiColorTextEdit" COMPONENT bin)

  # Install imgui fonts:
  file(GLOB IMGUIFONTS "Contrib/imgui/misc/fonts/*.ttf")
  install(FILES ${IMGUIFONTS} DESTINATION "${JEVOIS_MODULES_ROOT}/share/fonts" COMPONENT bin)

else()
  install(FILES "Config/videomappings-A33.cfg"
    DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin RENAME "videomappings.cfg")
endif()

# Copy threadpool header files to /jevois[pro]/include so jevoisbase and others can find them:
install(FILES Contrib/threadpool/libs/function2/include/function2/function2.hpp
  DESTINATION "${JEVOIS_MODULES_ROOT}/include/function2" COMPONENT bin)
FILE(GLOB CQFILES "Contrib/threadpool/libs/concurrentqueue/*.h")
install(FILES ${CQFILES}
  DESTINATION "${JEVOIS_MODULES_ROOT}/include" COMPONENT bin)

# Install jevois config files and scripts, shared files:
install(FILES "Config/params.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
install(FILES "Config/initscript.cfg"
  DESTINATION "${JEVOIS_MODULES_ROOT}/config" COMPONENT bin)
install(DIRECTORY "share-${JEVOIS}/"
  DESTINATION "${JEVOIS_MODULES_ROOT}/share" COMPONENT bin)

# Install libtensorflowlite.so library from Contrib. See Contrib/reinstall.sh for staging locations for each of host,
# platform, A33, Pro:
if (JEVOIS_PRO)

  if (JEVOIS_PLATFORM)
    install(FILES
      ${JEVOIS_BUILD_BASE}/usr/lib/libtensorflowlite.so
      DESTINATION "lib" COMPONENT bin)
  else (JEVOIS_PLATFORM)
    install(FILES
      /usr/lib/libtensorflowlite.so
      DESTINATION "/usr/lib" COMPONENT bin)
  endif (JEVOIS_PLATFORM)

else (JEVOIS_PRO)

  if (JEVOIS_PLATFORM)
    # libtensorflowlite is already in /var/lib/jevois-microsd from Contrib/reinstall.sh
  else (JEVOIS_PLATFORM)
    install(FILES /usr/lib/libtensorflowlite.so DESTINATION "/usr/lib" COMPONENT bin)
  endif (JEVOIS_PLATFORM)

endif (JEVOIS_PRO)

# Install the ovxlib and VX includes from Contrib, used by all configurations:
# Note: do not install /usr/include/VX on platform as it is already provided by aml-npu deb package:
file(GLOB JEVOIS_DETECT_LIB_FILES "Contrib/npu/detect_library/source_code/include/*.h")
install(FILES ${JEVOIS_DETECT_LIB_FILES} DESTINATION "${JEVOIS_MODULES_ROOT}/include" COMPONENT bin)

install(DIRECTORY "Contrib/npu/include/ovxlib" DESTINATION "include" COMPONENT bin)

if (JEVOIS_PRO)
  if (JEVOIS_PLATFORM)
  else ()
    # Install it for jevoispro-host:
    install(DIRECTORY "Contrib/npu/include/VX" DESTINATION "include" COMPONENT bin)
    install(DIRECTORY "Contrib/npu/include/CL" DESTINATION "include" COMPONENT bin)
    file(GLOB JEVOIS_VX_HOST_LIBS "Contrib/npu/x86_64_linux/lib/lib*")
    install(FILES ${JEVOIS_VX_HOST_LIBS} DESTINATION "/usr/lib" COMPONENT bin)
  endif ()
else()
  # and also for jevois-host and jevois-platform:
  install(DIRECTORY "Contrib/npu/include/VX" DESTINATION "include" COMPONENT bin)
  install(DIRECTORY "Contrib/npu/include/CL" DESTINATION "include" COMPONENT bin)
endif ()

########################################################################################################################
# Debian packaging:

# Create packages (Debian, RPM): in hbuild/ or pbuild/, just type 'sudo cpack' to create the package.
# To list the files created in a package, run: dpkg --contents <package.deb>
set(CPACK_PACKAGE_DESCRIPTION "JeVois Smart Machine Vision Core (${JEVOIS})")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JeVois Smart Embedded Machine Vision Toolkit, Core (${JEVOIS})")
set(CPACK_PACKAGE_CONTACT "Laurent Itti <jevois.org@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_DEBIAN_PACKAGE_SECTION "universe")
set(CPACK_PACKAGE_VENDOR "iLab at the University of Southern California")
set(CPACK_PACKAGE_VERSION_MAJOR "${JEVOIS_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${JEVOIS_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${JEVOIS_VERSION_PATCH}")
set(JEVOIS_PACKAGE_RELEASE "1") # packager revision number

set(CPACK_DEBIAN_PACKAGE_DEPENDS "build-essential, cmake (>= 3.1), gcc-${JEVOIS_COMPILER_VERSION}, \
g++-${JEVOIS_COMPILER_VERSION}, gfortran-${JEVOIS_COMPILER_VERSION}, \
guvcview, subversion, cmake, git, mercurial, doxygen-gui, graphviz, libboost-all-dev, ${TURBOJPEG_PKG}, \
autoconf, libeigen3-dev, screen, libgtk2.0-dev, libdc1394-22, libdc1394-22-dev, libjpeg-dev, libpng-dev, libtiff5-dev, \
libavcodec-dev, libavformat-dev, libswscale-dev, libxine2-dev, libgstreamer1.0-dev, libgstreamer-plugins-base1.0-dev, \
libv4l-dev, libtbb2, libtbb-dev, libfaac-dev, libmp3lame-dev, libopencore-amrnb-dev, \
libopencore-amrwb-dev, libtheora-dev, libvorbis-dev, libxvidcore-dev, x264, v4l-utils, unzip, qt5-default, \
libgtk-3-dev, libatlas-base-dev, libturbojpeg, libtbb2, libsdl2-dev, \
python${JEVOIS_HOST_PYTHON_MAJOR}.${JEVOIS_HOST_PYTHON_MINOR}-dev, python${JEVOIS_HOST_PYTHON_MAJOR}-numpy, \
python${JEVOIS_HOST_PYTHON_MAJOR}-pip, ${JEVOIS}-opencv (>=${JEVOIS_OPENCV_VERSION}), libglm-dev")

# Add cross-compiler dependencies on jevois-pro-host; on jevois-a33 it is provided by buildroot:
if (JEVOIS_PRO)

  # Add a few more depends needed by jevois pro host and platform:
  set(JVPRODEPS "${JEVOIS}-openvino, libedgetpu-dev, libedgetpu1-std, python3-pil, python3-tflite-runtime, \
                 python3-pycoral,")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "${JVPRODEPS} ${CPACK_DEBIAN_PACKAGE_DEPENDS}")

  if (JEVOIS_PLATFORM)
    # nothing on platform, compilers installed by fenix
  else ()
    # cross-compilers on host:
    set(platc "gcc-${JEVOIS_COMPILER_VERSION}-${CROSS_COMPILE}, \
             g++-${JEVOIS_COMPILER_VERSION}-${CROSS_COMPILE}, \
             gfortran-${JEVOIS_COMPILER_VERSION}-${CROSS_COMPILE},")
    string(REPLACE "-," "," platc ${platc})
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${platc} ${CPACK_DEBIAN_PACKAGE_DEPENDS}")
  endif ()
endif ()

# Use debian postinst / prerm to update ldconfig:
if (JEVOIS_PRO)
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/debian/prerm;")
endif()

# Use helper from JeVois.cmake for all other settings:
jevois_setup_cpack(${JEVOIS})
