# ---------------------------------------------------------------------------
# AFsp-v8r2 libtsp - audio file I/O library (static)
# ---------------------------------------------------------------------------
set(AFS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE TSP_SOURCES "${AFS_ROOT}/libtsp/*.c")

add_library(tsp STATIC ${TSP_SOURCES})
target_include_directories(tsp PUBLIC "${AFS_ROOT}/include")
target_compile_options(tsp PRIVATE -w)
