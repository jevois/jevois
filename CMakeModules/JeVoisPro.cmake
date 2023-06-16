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
else()
  message(FATAL_ERROR "Cannot find libboost-python, please apt install libboost-all-dev")
endif()

########################################################################################################################
# Python version installed on platform, fixed for now but may change later:
set(JEVOIS_PLATFORM_PYTHON_MAJOR 3)
set(JEVOIS_PLATFORM_PYTHON_MINOR 8)
set(JEVOIS_PLATFORM_PYTHON_M "")

set(JEVOIS_PLATFORM_BOOST_PYTHON "boost_python${JEVOIS_PLATFORM_PYTHON_MAJOR}${JEVOIS_PLATFORM_PYTHON_MINOR}")

########################################################################################################################
# Compiler flags optimized for platform:
set(JEVOIS_PLATFORM_ARCH_FLAGS "-march=armv8-a+crc -mcpu=cortex-a73.cortex-a53 -ftree-vectorize -Ofast -funsafe-math-optimizations")

########################################################################################################################
# Check for optional JEVOISPRO_SDK_ROOT environment variable, or use /usr/share/jevoispro-sdk by default:
if (DEFINED ENV{JEVOISPRO_SDK_ROOT})
  set(JEVOISPRO_SDK_ROOT $ENV{JEVOISPRO_SDK_ROOT})
else()
  set(JEVOISPRO_SDK_ROOT "/usr/share/${JEVOIS}-sdk")
endif()
message(STATUS "JeVoisPro SDK root: ${JEVOISPRO_SDK_ROOT}")

set(JEVOIS_BUILD_BASE "${JEVOISPRO_SDK_ROOT}/jevoispro-sysroot")
message(STATUS "JeVoisPro platform sysroot: ${JEVOIS_BUILD_BASE}")

########################################################################################################################
# Setup compilers to use on host:
set(JEVOIS_HOST_C_COMPILER "gcc-${JEVOIS_COMPILER_VERSION}")
set(JEVOIS_HOST_CXX_COMPILER "g++-${JEVOIS_COMPILER_VERSION}")
set(JEVOIS_HOST_FORTRAN_COMPILER "gfortran-${JEVOIS_COMPILER_VERSION}")

########################################################################################################################
# Setup the cross-compilers for the platform:
set(CROSS_COMPILE "aarch64-linux-gnu-")
set(JEVOIS_PLATFORM_C_COMPILER "${CROSS_COMPILE}gcc-${JEVOIS_COMPILER_VERSION}")
set(JEVOIS_PLATFORM_CXX_COMPILER "${CROSS_COMPILE}g++-${JEVOIS_COMPILER_VERSION}")
set(JEVOIS_PLATFORM_FORTRAN_COMPILER "${CROSS_COMPILE}gfortran-${JEVOIS_COMPILER_VERSION}")

########################################################################################################################
# OpenCV and other libraries on host and platform:

set(OPENCV_LIBS_FOR_JEVOIS "-lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_ml \
-lopencv_objdetect -lopencv_imgcodecs -lopencv_tracking -lopencv_video -lopencv_videoio -lopencv_dnn_objdetect \
-lopencv_dnn -lopencv_highgui")

# openvino libs for platform, from:
# /usr/share/jevoispro-sdk/jevoispro-sysroot/usr/share/jevoispro-openvino-2022.3/runtime/lib/aarch64/
set(JEVOIS_PLATFORM_OPENVINO_LIBS "-lopenvino_auto_batch_plugin -l:libopenvino_c.so.2230 \
-lopenvino_intel_myriad_plugin -l:libopenvino_onnx_frontend.so.2230 -l:libopenvino_paddle_frontend.so.2230 \
-l:libopenvino_tensorflow_frontend.so.2230 -lopenvino_auto_plugin -lopenvino_arm_cpu_plugin \
-lopenvino_gapi_preproc -l:libopenvino_ir_frontend.so.2230 -l:libopenvino.so.2230 -lopenvino_hetero_plugin")

# openvino libs for host, from /usr/share/jevoispro-openvino-2022.3/runtime/lib/intel64/
set(JEVOIS_HOST_OPENVINO_LIBS "-lopenvino -lopenvino_c -lopenvino_auto_plugin \
-lopenvino_onnx_frontend -lopenvino_intel_gna_plugin -lopenvino_intel_cpu_plugin \
-lopenvino_paddle_frontend -lopenvino_hetero_plugin -lopenvino_intel_gpu_plugin -lopenvino_intel_myriad_plugin \
-lopenvino_tensorflow_frontend -lopenvino_gapi_preproc -lopenvino_auto_batch_plugin")

# Ok, set the libs and paths for opencv and openvino:
set(JEVOIS_PLATFORM_OPENCV_PREFIX "${JEVOIS_BUILD_BASE}/usr/share/${JEVOIS}-opencv-${JEVOIS_OPENCV_VERSION}")
set(JEVOIS_PLATFORM_OPENVINO_PREFIX "${JEVOIS_BUILD_BASE}/usr/share/${JEVOIS}-openvino-${JEVOIS_OPENVINO_VERSION}")
set(JEVOIS_PLATFORM_OPENCV_INCLUDE "-I${JEVOIS_PLATFORM_OPENCV_PREFIX}/include/opencv4")
set(JEVOIS_PLATFORM_OPENCV_LIBS
  "-L${JEVOIS_BUILD_BASE}/usr/lib/aarch64-linux-gnu \
   -L${JEVOIS_PLATFORM_OPENCV_PREFIX}/lib \
   -L${JEVOIS_PLATFORM_OPENVINO_PREFIX}/runtime/lib/aarch64 \
   -L${JEVOIS_PLATFORM_OPENVINO_PREFIX}/runtime/3rdparty/tbb/lib \
   ${OPENCV_LIBS_FOR_JEVOIS} \
   ${JEVOIS_PLATFORM_OPENVINO_LIBS}")

set(JEVOIS_HOST_OPENCV_PREFIX "/usr/share/${JEVOIS}-opencv-${JEVOIS_OPENCV_VERSION}")
set(JEVOIS_HOST_OPENVINO_PREFIX "/usr/share/${JEVOIS}-openvino-${JEVOIS_OPENVINO_VERSION}")
set(JEVOIS_HOST_OPENCV_INCLUDE "-I${JEVOIS_HOST_OPENCV_PREFIX}/include/opencv4")
set(JEVOIS_HOST_OPENCV_LIBS
  "-L${JEVOIS_HOST_OPENCV_PREFIX}/lib \
   -L${JEVOIS_HOST_OPENVINO_PREFIX}/runtime/lib/intel64 \
   -L${JEVOIS_HOST_OPENVINO_PREFIX}/runtime/3rdparty/tbb/lib \
   ${OPENCV_LIBS_FOR_JEVOIS} \
   ${JEVOIS_HOST_OPENVINO_LIBS}")

# Use TBB and kernel includes for platform from the buildroot installation.  On the host, we may have local packages,
# eg, latest opencv compiled from source:
#set(KVER "4.9.241")
#set(JEVOIS_PLATFORM_KERNEL_INCLUDE "-I${JEVOIS_BUILD_BASE}/usr/src/linux-headers-${KVER}/include \
#    -I${JEVOIS_BUILD_BASE}/usr/src/linux-headers-${KVER}/arch/arm64/include \
#    -I${JEVOIS_BUILD_BASE}/usr/src/linux-headers-${KVER}/arch/arm64/include/generated")
set(JEVOIS_PLATFORM_TBB_INCLUDE "")

# Find python 3.x on host and platform:
set(JEVOIS_PLATFORM_PYTHON_INCLUDE
  "-I${JEVOIS_BUILD_BASE}/usr/include/python${JEVOIS_PLATFORM_PYTHON_MAJOR}.${JEVOIS_PLATFORM_PYTHON_MINOR}${JEVOIS_PLATFORM_PYTHON_M} -I${JEVOIS_BUILD_BASE}/usr/local/lib/python${JEVOIS_PLATFORM_PYTHON_MAJOR}.${JEVOIS_PLATFORM_PYTHON_MINOR}${JEVOIS_PLATFORM_PYTHON_M}/dist-packages/numpy/core/include/")
set(JEVOIS_PLATFORM_PYTHON_LIBS "-lpython${JEVOIS_PLATFORM_PYTHON_MAJOR}.${JEVOIS_PLATFORM_PYTHON_MINOR}${JEVOIS_PLATFORM_PYTHON_M} -l${JEVOIS_PLATFORM_BOOST_PYTHON}")

set(JEVOIS_HOST_PYTHON_INCLUDE
  "-I/usr/include/python${JEVOIS_HOST_PYTHON_MAJOR}.${JEVOIS_HOST_PYTHON_MINOR}${JEVOIS_HOST_PYTHON_M}")
set(JEVOIS_HOST_PYTHON_LIBS "-lpython${JEVOIS_HOST_PYTHON_MAJOR}.${JEVOIS_HOST_PYTHON_MINOR}${JEVOIS_HOST_PYTHON_M} -l${JEVOIS_HOST_BOOST_PYTHON}")

# We link against OpenGL on JeVois-Pro:
set(JEVOIS_HOST_OPENGL_LIBS "-lGLESv2 -lEGL -lX11 -lXext")
set(JEVOIS_PLATFORM_OPENGL_LIBS "-lGLESv2 -lEGL")

########################################################################################################################
# ImgGui library using SDL2 + OpenGL3 (we use ES 3.2) backend on host, custom + OpenGL3 on platform:
set(JEVOIS_HOST_OPENGL_LIBS "${JEVOIS_HOST_OPENGL_LIBS} -lSDL2")
set(JEVOIS_PLATFORM_OPENGL_LIBS "${JEVOIS_PLATFORM_OPENGL_LIBS}")
set(JEVOIS_HOST_SDL2_INCLUDE "-I/usr/include/SDL2")

########################################################################################################################
# Include path:
set(JEVOIS_PLATFORM_INCLUDE "-I${JEVOIS_BUILD_BASE}/usr/local/include -I${JEVOIS_BUILD_BASE}/usr/include ${JEVOIS_PLATFORM_OPENCV_INCLUDE} ${JEVOIS_PLATFORM_KERNEL_INCLUDE} ${JEVOIS_PLATFORM_TBB_INCLUDE} ${JEVOIS_PLATFORM_PYTHON_INCLUDE}")
set(JEVOIS_HOST_INCLUDE "${JEVOIS_HOST_OPENCV_INCLUDE} ${JEVOIS_HOST_PYTHON_INCLUDE} ${JEVOIS_HOST_SDL2_INCLUDE}")

