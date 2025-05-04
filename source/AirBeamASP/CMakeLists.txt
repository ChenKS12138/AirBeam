set(ABSL_PROPAGATE_CXX_STD ON)

set(DRIVER_NAME "AirBeamASP")


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

file(
  GLOB_RECURSE
  STANDALONE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*
)

# -----------------------------------------------------------------------------
# Resources
# -----------------------------------------------------------------------------

# Create a directory for resources
set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
file(MAKE_DIRECTORY "${RESOURCE_DIR}")

# List resource files (e.g., icons, localization files)
set(RESOURCE_FILES
    "${RESOURCE_DIR}/SidebarOpticalDisk.icns" # Example: Replace with your actual icon file
    # Add other resource files here
)

# Copy resources to the bundle
install(FILES ${RESOURCE_FILES}
    DESTINATION "Contents/Resources"
)

add_library(
  ${DRIVER_NAME}
  MODULE
  ${STANDALONE_FILES}
  ${RESOURCE_FILES} # Add resource files to the library
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
  MACOSX_BUNDLE_ICON_FILE "${RESOURCE_DIR}/SidebarOpticalDisk.icns" # Set the bundle icon
)
