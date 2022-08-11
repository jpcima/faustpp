cmake_minimum_required(VERSION 3.3)

project(faustpp)

option(FAUSTPP_USE_INTERNAL_BOOST "Use internal Boost libraries" OFF)
option(FAUSTPP_LINK_STATIC_BOOST "Link static Boost libraries" OFF)
option(FAUSTPP_BUILD_EXAMPLES "Build examples" OFF)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

# gsl
add_library(gsl INTERFACE)
target_include_directories(gsl
  INTERFACE "thirdparty/gsl")

# nonstd
add_library(nonstd INTERFACE)
target_include_directories(nonstd
  INTERFACE "thirdparty/optional-lite/include"
  INTERFACE "thirdparty/variant-lite/include"
  INTERFACE "thirdparty/string-view-lite/include"
  INTERFACE "thirdparty/expected-lite/include")

# fmt
add_library(fmt STATIC EXCLUDE_FROM_ALL
  "thirdparty/fmt/src/format.cc"
  "thirdparty/fmt/src/os.cc")
target_include_directories(fmt
  PUBLIC "thirdparty/fmt/include")

# pugixml
add_library(pugixml STATIC EXCLUDE_FROM_ALL
  "thirdparty/pugixml/pugixml.cpp")
target_include_directories(pugixml
  PUBLIC "thirdparty/pugixml")

# boost
set(Boost_USE_STATIC_LIBS "${FAUSTPP_LINK_STATIC_BOOST}")

if(${FAUSTPP_USE_INTERNAL_BOOST})
set(BOOST_ENABLE_CMAKE ON)
set(BOOST_DIR ${PROJECT_SOURCE_DIR}/thirdparty/jinja2cpp/thirdparty/boost)
set(BOOST_INCLUDE_LIBRARIES
    algorithm
    assert
    atomic
    filesystem
    lexical_cast
    optional
    variant
CACHE INTERNAL "")
add_subdirectory(${BOOST_DIR})
set(Boost_INCLUDE_DIRS
  ${BOOST_DIR}/libs/algorithm/include
  ${BOOST_DIR}/libs/container/include
  ${BOOST_DIR}/libs/intrusive/include
  ${BOOST_DIR}/libs/numeric/conversion/include
  ${BOOST_DIR}/libs/range/include
  ${BOOST_DIR}/libs/unordered/include
  ${BOOST_DIR}/libs/variant/include
)
set(Boost_SYSTEM_LIBRARY Boost::system)
set(Boost_FILESYSTEM_LIBRARY Boost::filesystem)
else()
find_package(Boost REQUIRED COMPONENTS "filesystem" "system")
endif()

add_library(boost INTERFACE)
target_include_directories(boost
  INTERFACE ${Boost_INCLUDE_DIRS})

add_library(boost-system INTERFACE)
target_link_libraries(boost-system
  INTERFACE "${Boost_SYSTEM_LIBRARY}"
  INTERFACE boost)

add_library(boost-filesystem INTERFACE)
target_link_libraries(boost-filesystem
  INTERFACE "${Boost_FILESYSTEM_LIBRARY}"
  INTERFACE boost)

# rapidjson
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson
  INTERFACE "thirdparty/rapidjson/include")

# jinja2
add_library(jinja2 STATIC EXCLUDE_FROM_ALL
  "thirdparty/jinja2cpp/src/internal_value.cpp"
  "thirdparty/jinja2cpp/src/expression_evaluator.cpp"
  "thirdparty/jinja2cpp/src/template.cpp"
  "thirdparty/jinja2cpp/src/filters.cpp"
  "thirdparty/jinja2cpp/src/error_info.cpp"
  "thirdparty/jinja2cpp/src/testers.cpp"
  "thirdparty/jinja2cpp/src/value.cpp"
  "thirdparty/jinja2cpp/src/statements.cpp"
  "thirdparty/jinja2cpp/src/rapid_json_serializer.cpp"
  "thirdparty/jinja2cpp/src/string_converter_filter.cpp"
  "thirdparty/jinja2cpp/src/serialize_filters.cpp"
  "thirdparty/jinja2cpp/src/lexer.cpp"
  "thirdparty/jinja2cpp/src/template_parser.cpp"
  "thirdparty/jinja2cpp/src/expression_parser.cpp"
  "thirdparty/jinja2cpp/src/filesystem_handler.cpp"
  "thirdparty/jinja2cpp/src/template_env.cpp"
  "thirdparty/jinja2cpp/src/generic_list.cpp")
target_include_directories(jinja2
  PUBLIC "thirdparty/jinja2cpp/include")
target_link_libraries(jinja2
  PUBLIC nonstd
  PUBLIC fmt
  PUBLIC rapidjson
  PUBLIC boost-filesystem
  PUBLIC boost-system)

# faustpp
add_executable(faustpp
  "main.cpp"
  "call_faust.cpp"
  "metadata.cpp")
target_link_libraries(faustpp
  PRIVATE gsl
  PRIVATE nonstd
  PRIVATE fmt
  PRIVATE pugixml
  PRIVATE boost
  PRIVATE jinja2
  PRIVATE "${CMAKE_THREAD_LIBS_INIT}")

# faustpp examples
if(FAUSTPP_BUILD_EXAMPLES)
  add_subdirectory("examples")
endif()

# installation
include(GNUInstallDirs)
install(TARGETS faustpp
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
