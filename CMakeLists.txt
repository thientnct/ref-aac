cmake_minimum_required(VERSION 3.16)
project(refaac_c C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall -W -Wno-unused -Wno-long-long -Wmissing-prototypes -fcommon")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast -march=native")

add_subdirectory(mp4_file_format/libisomediafile)
add_subdirectory(AFsp-v8r2)
add_subdirectory(mp4AudVm_Rewrite)
add_subdirectory(mp4AudVm)
add_subdirectory(mp4mcEnc)
add_subdirectory(mp4MultChEnc980508)
add_subdirectory(v1refsoft990809)
