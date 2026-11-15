# ---------------------------------------------------------------------------
# libisomediafile (MP4FF) - Windows (w32) platform objects
# ---------------------------------------------------------------------------
set(ISOMEDIA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB ISOMEDIA_SOURCES "${ISOMEDIA_ROOT}/src/*.c")
list(APPEND ISOMEDIA_SOURCES "${ISOMEDIA_ROOT}/w32/W32FileMappingObject.c")

add_library(isomediafile STATIC ${ISOMEDIA_SOURCES})
target_include_directories(isomediafile PUBLIC
  "${ISOMEDIA_ROOT}/src"
  "${ISOMEDIA_ROOT}/w32"
)
target_compile_options(isomediafile PRIVATE -w)
