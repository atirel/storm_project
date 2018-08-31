cmake_minimum_required(VERSION 3.1)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LLVM_DIR})
include(${CMAKE_MODULE_PATH}/LLVM-Config.cmake)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include(${LLVM_DIR}/LLVM-Config.cmake)

include(HandleLLVMOptions)
include(AddLLVM)


link_directories(${LLVM_LIBRARY_DIRS})

#add_subdirectory(DoubleStore)
add_subdirectory(DeadVariableHandler)
add_subdirectory(Initialize)
add_subdirectory(PutAtZero)
