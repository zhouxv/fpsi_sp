cmake_minimum_required(VERSION 3.10)
project(unbalanced-FPSI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

include_directories(include)

file(GLOB SOURCES src/*.cpp)

add_executable(ufpsi ${SOURCES})

# secureJoin

target_include_directories(ufpsi PRIVATE
  /usr/local/include/secureJoin
)

find_package(cryptoTools REQUIRED)
find_package(libOTe REQUIRED)
find_package(Kuku 2.1 QUIET REQUIRED)

target_link_libraries(ufpsi PRIVATE secureJoin oc::cryptoTools libOTe)

# PIRANA

add_subdirectory(thirdparty/PIRANA)
target_include_directories(ufpsi PRIVATE thirdparty/PIRANA/src)
target_link_libraries(ufpsi PRIVATE pirana)

# OKVS
add_subdirectory(thirdparty/OKVS)
target_include_directories(ufpsi PRIVATE thirdparty/OKVS)
target_link_libraries(ufpsi PRIVATE okvs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
  target_compile_options(ufpsi PRIVATE -mpclmul -maes -msse2 -msse4.1)
endif()