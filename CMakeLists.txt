cmake_minimum_required(VERSION 3.10)
project(FPSI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

include_directories(include)

file(GLOB SOURCES src/*.cpp)

add_executable(fpsi ${SOURCES})

# secureJoin

target_include_directories(fpsi PRIVATE
  /usr/local/include/secureJoin
)

find_package(cryptoTools REQUIRED)
find_package(libOTe REQUIRED)
find_package(volePSI REQUIRED)
find_package(Kuku 2.1 QUIET REQUIRED)

target_link_libraries(fpsi PRIVATE secureJoin oc::cryptoTools libOTe visa::volePSI)

# PIRANA

add_subdirectory(thirdparty/PIRANA)
target_include_directories(fpsi PRIVATE thirdparty/PIRANA/src)
target_link_libraries(fpsi PRIVATE pirana)

# OKVS
add_subdirectory(thirdparty/OKVS)
target_include_directories(fpsi PRIVATE thirdparty/OKVS)
target_link_libraries(fpsi PRIVATE okvs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
  target_compile_options(fpsi PRIVATE -mpclmul -maes -msse2 -msse4.1)
endif()

add_definitions(-DNDEBUG)
