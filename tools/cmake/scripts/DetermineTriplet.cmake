# A small script to determine the target triplet to be used by vcpkg. Note that it relies on certain CMake variables
# being passed on the command line, e.g.: cmake -DTEIDE_SANITIZER=OFF -P tools/cmake/scripts/DetermineTriplet.cmake

include("${CMAKE_CURRENT_LIST_DIR}/../Triplet.cmake")

execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${VCPKG_TARGET_TRIPLET}")
