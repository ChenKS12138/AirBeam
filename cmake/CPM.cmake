# setup cpm
set(CPM_DOWNLOAD_LOCATION ${CMAKE_BINARY_DIR}/CPM.cmake)
file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.41.0/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION})
include(${CPM_DOWNLOAD_LOCATION})
