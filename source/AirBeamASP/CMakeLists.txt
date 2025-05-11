set(DRIVER_NAME "AirBeamASP")
set(DRIVER_VERSION "1.0.0")
set(DRIVER_COPYRIGHT "Copyright (c) chenks12138 authors")
set(DRIVER_IDENTIFIER "airbeam.cattchen.top")
set(DRIVER_UID "6BEC3E7D-8A3A-4E6D-8B9F-1F5B8A6C7D4A")
set(DRIVER_ENTRYPOINT "AirBeamASPEntryPoint")


if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
    option(SIMD_ARM "Enable SIMD ARM optimizations" ON)
    message("SIMD_ARM is ON")
else()
    option(SIMD_ARM "Enable SIMD ARM optimizations" OFF)
    message("SIMD_ARM is OFF")
endif()

if(SIMD_ARM)
  add_definitions(-DSIMD_ARM)
endif()

# Add definition for Release build
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DBUILD_DEBUG)
  message("Adding BUILD_DEBUG definition")
endif()

file(
  GLOB_RECURSE
  STANDALONE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

add_library(
  ${DRIVER_NAME}
  MODULE
  ${STANDALONE_FILES}
)

find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)

set(ABSL_PROPAGATE_CXX_STD ON)
CPMAddPackage("gh:abseil/abseil-cpp#20230125.3")
CPMAddPackage("gh:fmtlib/fmt#7.1.3")
CPMAddPackage("gh:gavv/libASPL#v3.1.1")

target_link_libraries(
  ${DRIVER_NAME}
  libASPL
  fmt
  ${COREAUDIO_LIBRARY}

  absl::strings
  absl::base
)


set_target_properties(${DRIVER_NAME} PROPERTIES
  OUTPUT_NAME "${DRIVER_NAME}"
  BUNDLE true
  BUNDLE_EXTENSION "driver"
  PREFIX ""
  SUFFIX ""
  MACOSX_BUNDLE ON
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
  MACOSX_BUNDLE_BUNDLE_NAME "${DRIVER_NAME}"
  MACOSX_BUNDLE_BUNDLE_VERSION "${DRIVER_VERSION}"
  MACOSX_BUNDLE_COPYRIGHT "${DRIVER_COPYRIGHT}"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${DRIVER_IDENTIFIER}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${DRIVER_VERSION}"
)
