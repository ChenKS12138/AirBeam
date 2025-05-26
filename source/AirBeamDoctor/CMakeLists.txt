SET(AIRBEAM_DOCTOR_TARGET AirBeamDoctor)

file(
  GLOB_RECURSE
  AIRBEAM_DOCTOR_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

add_executable(
  ${AIRBEAM_DOCTOR_TARGET}
  ${AIRBEAM_DOCTOR_FILES}
)

target_include_directories(
  ${AIRBEAM_DOCTOR_TARGET} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(ABSL_PROPAGATE_CXX_STD ON)
CPMAddPackage("gh:abseil/abseil-cpp#20250512.0")

target_link_libraries(
  ${AIRBEAM_DOCTOR_TARGET}

  absl::flags
  absl::flags_parse
  absl::log
  absl::log_initialize
  absl::strings

  AirBeamCore
)
