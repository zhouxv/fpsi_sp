cmake_minimum_required(VERSION 3.13)
project(PIRexamples VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE RELWITHDEBINFO)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SEAL 4.1.2 REQUIRED)
find_package(Kuku 2.1 QUIET REQUIRED)

if(NOT Kuku_FOUND)
    message(FATAL_ERROR "Microsoft Kuku: not found")
else()
    message(STATUS "Microsoft Kuku: found")
endif()

find_package(libOTe REQUIRED)

# set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../bin)

# add_executable(pirexamples)
# target_sources(pirexamples
#     PRIVATE
#         ${CMAKE_CURRENT_LIST_DIR}/src/main.cc
#         ${CMAKE_CURRENT_LIST_DIR}/src/utils.cc
#         ${CMAKE_CURRENT_LIST_DIR}/src/pir_parms.cc
#         ${CMAKE_CURRENT_LIST_DIR}/src/server.cc
#         ${CMAKE_CURRENT_LIST_DIR}/src/client.cc
#         ${CMAKE_CURRENT_LIST_DIR}/src/test.cc
# )
# if(TARGET SEAL::seal)
#     target_link_libraries(pirexamples PRIVATE SEAL::seal)
# else()
#     message(FATAL_ERROR "Cannot find target SEAL::seal or SEAL::seal_shared")
# endif()

# target_link_libraries(pirexamples PRIVATE Kuku::kuku)

# set_target_properties(pirexamples  PROPERTIES
# RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

add_library(pirana SHARED 
    ${CMAKE_CURRENT_LIST_DIR}/src/utils.cc
    ${CMAKE_CURRENT_LIST_DIR}/src/pir_parms.cc
    ${CMAKE_CURRENT_LIST_DIR}/src/server.cc
    ${CMAKE_CURRENT_LIST_DIR}/src/client.cc
    ${CMAKE_CURRENT_LIST_DIR}/src/test.cc
)

target_include_directories(pirana PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/src
)

if(TARGET SEAL::seal)
    target_link_libraries(pirana PUBLIC SEAL::seal)
else()
    message(FATAL_ERROR "Cannot find target SEAL::seal or SEAL::seal_shared")
endif()

target_link_libraries(pirana PUBLIC Kuku::kuku oc::libOTe)