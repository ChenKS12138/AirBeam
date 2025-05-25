SET(AIRBEAM_CORE_TARGET AirBeamCore)

file(
  GLOB_RECURSE
  AIRBEAM_CORE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

add_library(
  ${AIRBEAM_CORE_TARGET}
  ${AIRBEAM_CORE_FILES}
)

target_include_directories(
  ${AIRBEAM_CORE_TARGET} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(ABSL_PROPAGATE_CXX_STD ON)
CPMAddPackage("gh:abseil/abseil-cpp#20230125.3")
CPMAddPackage("gh:fmtlib/fmt#7.1.3")

target_link_libraries(
  ${AIRBEAM_CORE_TARGET}

  fmt
  absl::strings
  absl::base
)
