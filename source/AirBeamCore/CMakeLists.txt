SET(AIRBEAM_CORE_TARGET AirBeamCore)
SET(AIRBEAM_CORE_TARGET_DEBUG ${AIRBEAM_CORE_TARGET}_DEBUG)

option(BUILD_DEBUG "Enable debug build" OFF)
if(BUILD_DEBUG)
  add_definitions(-DBUILD_DEBUG)
endif()


set(ABSL_PROPAGATE_CXX_STD ON)
CPMAddPackage("gh:abseil/abseil-cpp#20250512.0")
CPMAddPackage("gh:fmtlib/fmt#7.1.3")

file(
  GLOB_RECURSE
  AIRBEAM_CORE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

# Create a macro to handle common target configurations
macro(configure_airbeam_target target_name)
  add_library(
    ${target_name}
    ${AIRBEAM_CORE_FILES}
  )

  target_include_directories(
    ${target_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(
    ${target_name}
    fmt::fmt
    absl::strings
    absl::base
  )
endmacro()

# Configure both targets using the macro
configure_airbeam_target(${AIRBEAM_CORE_TARGET})
configure_airbeam_target(${AIRBEAM_CORE_TARGET_DEBUG})

target_compile_definitions(
  ${AIRBEAM_CORE_TARGET_DEBUG}
  PUBLIC
  -DBUILD_DEBUG
)