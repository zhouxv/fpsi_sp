cmake_minimum_required(VERSION 3.16)
project(cmakeTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



add_definitions(-DLIBDIVIDE_AVX2)


add_library(okvs STATIC
    ${CMAKE_CURRENT_LIST_DIR}/OKVS.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SimpleIndex.cpp
)

find_package(libOTe REQUIRED)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# target_include_directories(okvs PUBLIC 
#     ${CMAKE_CURRENT_LIST_DIR}/
# )


target_compile_options(okvs PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++20> -lpthread)

#target_link_libraries(main oc::libOTe)

target_link_libraries(okvs PUBLIC oc::libOTe)
# add_definitions(-DNDEBUG)


# set any other operties like cpp version


# cmake_minimum_required(VERSION 3.16)
# project(cmakeTest)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_definitions(-DLIBDIVIDE_AVX2)

# add_executable(main main.cpp SimpleIndex.cpp OKVS.cpp)

# find_package(libOTe REQUIRED)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)
# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)


# target_compile_options(main PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++20> -lpthread)

# #target_link_libraries(main oc::libOTe)

# target_link_libraries(main oc::libOTe)

# # set any other operties like cpp version
