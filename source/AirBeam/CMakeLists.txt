include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)

file(
  GLOB_RECURSE
  SOURCE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

add_executable(AirBeam MACOSX_BUNDLE ${SOURCE_FILES})

target_link_libraries(AirBeam
  "-framework Cocoa"
  "-framework Security"
)

# Add AirBeamASP as a target dependency
add_dependencies(AirBeam AirBeamASP)

set_target_properties(AirBeam PROPERTIES
  MACOSX_BUNDLE_BUNDLE_NAME "AirBeam"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.AirBeam"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
  MACOSX_BUNDLE_VERSION "1.0"
  MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2024, Example. All rights reserved."
  MACOSX_BUNDLE_ICON_FILE "iconfile"
)

# Copy AirBeamASP.driver to AirBeam.app/Contents/Resources
add_custom_command(TARGET AirBeam
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                           "${CMAKE_BINARY_DIR}/source/AirBeamASP/AirBeamASP.driver"
                           "${CMAKE_CURRENT_BINARY_DIR}/AirBeam.app/Contents/Resources/AirBeamASP.driver")

add_custom_command(TARGET AirBeam
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy
                           "${CMAKE_CURRENT_SOURCE_DIR}/Resources/iconfile.icns"
                           "${CMAKE_CURRENT_BINARY_DIR}/AirBeam.app/Contents/Resources/iconfile.icns")
