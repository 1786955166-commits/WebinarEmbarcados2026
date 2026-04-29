# Install script for directory: /home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/jaga/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/jaga/Workspace/work/27-GZM/WebinarEmbarcados2026/app/build/modules/picolibc/newlib/libc/include/sys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/jaga/Workspace/work/27-GZM/WebinarEmbarcados2026/app/build/modules/picolibc/newlib/libc/include/machine/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/jaga/Workspace/work/27-GZM/WebinarEmbarcados2026/app/build/modules/picolibc/newlib/libc/include/ssp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/jaga/Workspace/work/27-GZM/WebinarEmbarcados2026/app/build/modules/picolibc/newlib/libc/include/rpc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/jaga/Workspace/work/27-GZM/WebinarEmbarcados2026/app/build/modules/picolibc/newlib/libc/include/arpa/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/alloca.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/argz.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/ar.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/assert.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/byteswap.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/cpio.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/ctype.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/devctl.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/dirent.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/endian.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/envlock.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/envz.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/errno.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/fastmath.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/fcntl.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/fenv.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/fnmatch.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/getopt.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/glob.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/grp.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/iconv.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/ieeefp.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/inttypes.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/langinfo.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/libgen.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/limits.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/locale.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/malloc.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/math.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/memory.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/ndbm.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/newlib.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/paths.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/picotls.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/pwd.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/regdef.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/regex.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/sched.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/search.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/setjmp.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/signal.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/spawn.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/stdint.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/stdnoreturn.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/stdlib.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/string.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/strings.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/tar.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/termios.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/time.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/uchar.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/unctrl.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/unistd.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/utime.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/utmp.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/wchar.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/wctype.h"
    "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/wordexp.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/jaga/zephyrproject/modules/lib/picolibc/newlib/libc/include/complex.h")
endif()

