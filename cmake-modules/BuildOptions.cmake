include(CMakeDependentOption)

# Components to build
option(BUILD_TESTS "Build with unittests" ON)
option(USE_SYSTEM_BOOST "Use system boost libraries" ON)
option(USE_SYSTEM_THRIFT "Use system thrift library" ON)
option(USE_SYSTEM_JEMALLOC "Use system Jemalloc" ON)
option(USE_SYSTEM_LIBEVENT "Use system libevent" ON)
option(GENERATE_THRIFT "Generate thrift files" OFF)

message(STATUS "----------------------------------------------------------")
message(STATUS "${PROJECT_NAME} version:                            ${PROJECT_VERSION}")
message(STATUS "Build configuration Summary")
message(STATUS "  Build unit tests:                       ${BUILD_TESTS}")
message(STATUS "  Use system Boost library:               ${USE_SYSTEM_BOOST}")
message(STATUS "  Use system Thrift library:              ${USE_SYSTEM_THRIFT}")
message(STATUS "  Use system Jemalloc:                    ${USE_SYSTEM_JEMALLOC}")
message(STATUS "  Use system libevent:                    ${USE_SYSTEM_LIBEVENT}")
message(STATUS "  Generate thrift files:                  ${GENERATE_THRIFT}")
message(STATUS "----------------------------------------------------------")
