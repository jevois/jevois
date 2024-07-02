Variables used in the JeVois CMake files
========================================

The description below will help you understand our CMake code. Most users can just ignore this.

Basic variables
---------------

- JEVOIS_HARDWARE [A33|PRO] - defined by cmake caller to determine whether to build for JeVois-A33 or JeVois-Pro

- JEVOIS_VENDOR [string] - name (starting with uppercase letter) of a vendor, used to group machine vision modules into
  different sources (each vendor translates to a directory under /jevois/modules/)

- JEVOIS_PLATFORM [ON|OFF] - whether to compile natively for host, or cross-compile for platform hardware

- JEVOIS_A33 [ON|OFF] - convenience shortcut for JEVOIS_HARDWARE==A33

- JEVOIS_PRO [ON|OFF] - convenience shortcut for JEVOIS_HARDWARE==PRO

- JEVOIS [string] - evaluates to "jevois" or "jevoispro" depending on JEVOIS_HARDWARE

- JEVOIS_NATIVE [ON|OFF] - set by cmake caller when compiling a module on a running JeVois-Pro camera. This will trigger
  using aarch64 compilation flags, yet using the native compiler (not cross-compiler), and will set various library
  paths and others for native compilation on JeVois-Pro


Variables with variants depending on compilation target
-------------------------------------------------------

Some variables have HOST, PLATFORM, and possibly PLATFORM_NATIVE variants. The variants are defined during initial
setup. Then a final variable without any variant is set to either the HOST, PLATFORM, or PLATFORM_NATIVE variant
depending on the value of JEVOIS_HARDWARE.

For example: compiler flags vary depending on the compilation target:
- JEVOIS_HOST_CFLAGS - flags optimized for x86_64 compilation when running rebuild-host.sh
- JEVOIS_PLATFORM_CFLAGS - flags optimized for cross-compilation to arm/aarch64 when running rebuild-platform.sh
- JEVOIS_PLATFORM_NATIVE_CFLAGS - flags optimized for aarch64 compilation when compiling on a running JeVois-Pro

One of these variants will end up in JEVOIS_CFLAGS depending on JEVOIS_HARDWARE and JEVOIS_NATIVE values. Downstream
CMake rules would typically only use JEVOIS_CFLAGS.

