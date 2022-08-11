include(FindPkgConfig)

pkg_check_modules(JACK "jack" REQUIRED)

link_directories("${JACK_LIBRARY_DIRS}")

macro(add_example NAME)
  add_executable("${NAME}"
    "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.jack.cpp")
  target_include_directories("${NAME}"
    PRIVATE "${JACK_INCLUDE_DIRS}")
  target_link_libraries("${NAME}"
    PRIVATE "${JACK_LIBRARIES}")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
    COMMAND "$<TARGET_FILE:faustpp>" -a "${PROJECT_SOURCE_DIR}/architectures/generic.cpp"
            "-DIdentifier=${NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
            "-o" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cpp")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.hpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
    COMMAND "$<TARGET_FILE:faustpp>" -a "${PROJECT_SOURCE_DIR}/architectures/generic.hpp"
            "-DIdentifier=${NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
            "-o" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.hpp")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.jack.cpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
    COMMAND "$<TARGET_FILE:faustpp>" -a "${PROJECT_SOURCE_DIR}/architectures/jack_simple.cpp"
            "-DIdentifier=${NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
            "-o" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.jack.cpp")
endmacro()

macro(add_oversampled_example NAME)
  foreach(FACTOR 1;2;4;8;16)
    add_executable("${NAME}${FACTOR}X"
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.hpp"
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.jack.cpp")
    target_include_directories("${NAME}${FACTOR}X"
      PRIVATE "${JACK_INCLUDE_DIRS}"
      PRIVATE "${PROJECT_SOURCE_DIR}/thirdparty/hiir")
    target_link_libraries("${NAME}${FACTOR}X"
      PRIVATE "${JACK_LIBRARIES}")
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.cpp"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
      COMMAND "$<TARGET_FILE:faustpp>" -a "${PROJECT_SOURCE_DIR}/architectures/oversampled.cpp"
              "-DIdentifier=${NAME}${FACTOR}X" "-DOversampling=${FACTOR}" "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
              "-o" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.cpp")
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.hpp"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
      COMMAND "$<TARGET_FILE:faustpp>" -a "${PROJECT_SOURCE_DIR}/architectures/oversampled.hpp"
              "-DIdentifier=${NAME}${FACTOR}X" "-DOversampling=${FACTOR}" "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
              "-o" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.hpp")
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.jack.cpp"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
      COMMAND "$<TARGET_FILE:faustpp>" -a "${PROJECT_SOURCE_DIR}/architectures/jack_simple.cpp"
              "-DIdentifier=${NAME}${FACTOR}X" "-DOversampling=${FACTOR}" "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.dsp"
              "-o" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}${FACTOR}X.jack.cpp")
  endforeach()
endmacro()

add_example(stone_phaser)
add_example(stone_phaser_stereo)
add_oversampled_example(osctriangle)
add_oversampled_example(hardclip)
