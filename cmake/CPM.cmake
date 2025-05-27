set(CPM_DOWNLOAD_LOCATION ${CMAKE_BINARY_DIR}/CPM.cmake)

file(DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.41.0/CPM.cmake
    ${CPM_DOWNLOAD_LOCATION}
    SHOW_PROGRESS
    STATUS download_status
    )

list(GET download_status 0 status_code)
list(GET download_status 1 status_string)

if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "Failed to download CPM.cmake: ${status_string}")
endif()

include(${CPM_DOWNLOAD_LOCATION})
