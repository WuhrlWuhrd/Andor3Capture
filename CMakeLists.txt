cmake_minimum_required(VERSION 3.13) 

find_package(SWIG 4.0 COMPONENTS python)
if(SWIG_FOUND)
  message("SWIG found: ${SWIG_EXECUTABLE}")
endif()

project(Andor3Capture)               
set(CMAKE_CXX_STANDARD 17)           

include_directories(include ${CMAKE_CURRENT_LIST_DIR})
include_directories(/usr/include/python3.11 /usr/include/python3.11)

add_executable(Andor3Capture main.cpp)
target_link_libraries(Andor3Capture ${CMAKE_CURRENT_LIST_DIR}/libatcore.so.3.10.30003.5)


INCLUDE(${SWIG_USE_FILE})

SET_SOURCE_FILES_PROPERTIES(A3C.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(A3C.i PROPERTIES SWIG_FLAGS "-includeall")
SET_SOURCE_FILES_PROPERTIES(A3C.i PROPERTIES OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/build/PyAndor3Capture)
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_LIST_DIR}/build/PyAndor3Capture)
set_property(SOURCE A3C.i PROPERTY CPLUSPLUS ON)
swig_add_module(PyAndor3Capture python A3C.i ${SOURCES})
swig_link_libraries(PyAndor3Capture ${CMAKE_CURRENT_LIST_DIR}/libatcore.so.3.10.30003.5)
set_target_properties(PyAndor3Capture PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})