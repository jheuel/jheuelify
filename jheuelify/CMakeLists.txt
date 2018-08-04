cmake_minimum_required(VERSION 3.2)

project(jheuelify)
set(PROJECT_VERSION 0.1.0.0)
set(PROJECT_BRIEF "\"Telegram bot that replaces faces in images of incoming messages.\"")
#set(CMAKE_VERBOSE_MAKEFILE on)

# Enable/disable testing
option(TESTS_ENABLED "Enable automatic tests" ON)
# Enable/disable test coverage
option(COVERAGE_ENABLED "Enable test coverage" OFF)



set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

include_directories(${CMAKE_SOURCE_DIR}/include)

if(COVERAGE_ENABLED)
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -fPIC -O0 -ggdb -fprofile-arcs -ftest-coverage")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -O0 -ggdb")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall")


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()


set(LIBRARIES ${LIBRARIES} curl)
set(LIBRARIES ${LIBRARIES} curlpp)
set(LIBRARIES ${LIBRARIES} TgBot)
set(LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

find_package(dlib REQUIRED)
set(LIBRARIES ${LIBRARIES} dlib::dlib)

find_package(Boost COMPONENTS
    program_options
    filesystem
    thread
    system
    iostreams
    REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
set(LIBRARIES ${LIBRARIES} ${Boost_LIBRARIES})
set(Boost_USE_MULTITHREADED ON)

find_package(Eigen3 REQUIRED eigen3)
include_directories(${EIGEN3_INCLUDE_DIR})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${OpenCV_LIBS})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /opt/OpenBLAS/lib/cmake/openblas)
find_package(OpenBLAS REQUIRED)
set(LIBRARIES ${LIBRARIES} ${OpenBLAS_LIBRARIES})
include_directories(${OpenBLAS_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
set(LIBRARIES ${LIBRARIES} ${OPENSSL_LIBRARIES})

if(TESTS_ENABLED)
    enable_testing()
    add_subdirectory(test)
endif()

add_subdirectory(src)

# vim:ft=cmake
