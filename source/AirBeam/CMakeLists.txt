include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)

file(
  GLOB_RECURSE
  SOURCE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

add_executable(AirBeam MACOSX_BUNDLE ${SOURCE_FILES})

target_link_libraries(AirBeam
  "-framework Cocoa"
)

set_target_properties(AirBeam PROPERTIES
  MACOSX_BUNDLE_BUNDLE_NAME "AirBeam"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.AirBeam"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
  MACOSX_BUNDLE_VERSION "1.0"
)