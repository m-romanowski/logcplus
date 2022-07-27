cmake_minimum_required(VERSION 3.22)
project(logcplus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(LIBS ${CMAKE_SOURCE_DIR}/lib)
set(SOURCES ${CMAKE_SOURCE_DIR}/src)
set(TESTS ${CMAKE_SOURCE_DIR}/test)

find_package(Threads REQUIRED)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

include_directories(${LIBS})
include_directories(${LIBS}/PollingConditions/src)
include_directories(${SOURCES})
include_directories(${TESTS})

set(SOURCE_FILES
        ${LIBS}/PollingConditions/src/predefinedpollingconditions.h
        ${SOURCES}/logcplus.h
        ${TESTS}/testsfixture.h
        ${TESTS}/loggertest.cpp)

add_executable(logcplusTests ${SOURCE_FILES})
target_link_libraries(logcplusTests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
