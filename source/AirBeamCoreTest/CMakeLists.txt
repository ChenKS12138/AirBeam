option(BUILD_TEST "Enable building tests" ON)

if(BUILD_TEST)
  message(STATUS "Building tests")
else()
  message(STATUS "Skipping tests")
  return()
endif()

SET(AIRBEAM_CORE_TEST_TARGET AirBeamCoreTest)
SET(AIRBEAM_CORE_TEST_BIN "_${AirBeamCoreTest}")

CPMAddPackage("gh:google/googletest#v1.17.0")

file(GLOB_RECURSE TEST_SOURCE_FILE ${CMAKE_CURRENT_LIST_DIR}/*)

add_executable(
  ${AIRBEAM_CORE_TEST_BIN}

  ${TEST_SOURCE_FILE}
)

target_link_libraries(
  ${AIRBEAM_CORE_TEST_BIN}
  AirBeamCore

  gtest
  gtest_main
)

add_custom_target(
  ${AIRBEAM_CORE_TEST_TARGET}
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${AIRBEAM_CORE_TEST_BIN}
  DEPENDS ${AIRBEAM_CORE_TEST_BIN}
)
