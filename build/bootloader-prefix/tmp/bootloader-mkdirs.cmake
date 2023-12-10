# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/Progetti/esp-idf/components/bootloader/subproject"
  "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader"
  "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix"
  "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix/tmp"
  "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix/src"
  "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Progetti/esp-workspace/temperatureSensor/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
