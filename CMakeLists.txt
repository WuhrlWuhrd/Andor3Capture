cmake_minimum_required(VERSION 3.13) 

# If SWIG is available, use it to generate the PyAndor3Capture python wrapper
find_package(SWIG 4.0 COMPONENTS python)
if(SWIG_FOUND)

  message("SWIG found: ${SWIG_EXECUTABLE}")
  INCLUDE(${SWIG_USE_FILE})
  
  find_package(PythonLibs REQUIRED)
  include_directories(${PYTHON_INCLUDE_DIRS})

  SET_SOURCE_FILES_PROPERTIES(A3C.i PROPERTIES CPLUSPLUS ON)
  SET_SOURCE_FILES_PROPERTIES(A3C.i PROPERTIES SWIG_FLAGS "-includeall")
  SET_SOURCE_FILES_PROPERTIES(A3C.i PROPERTIES OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/build/PyAndor3Capture)

  set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_LIST_DIR}/build/PyAndor3Capture)
  set_property(SOURCE A3C.i PROPERTY CPLUSPLUS ON)
  swig_add_module(PyAndor3Capture python A3C.i ${SOURCES})
  swig_link_libraries(PyAndor3Capture atcore)
  swig_link_libraries(PyAndor3Capture atutility)
  set_target_properties(PyAndor3Capture PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})

else()
  message("SWIG NOT FOUND! No Python module will be compiled as a result.")
endif()

find_file(LIB_CORE NAMES atcore.dll libatcore.so libatcore.so.*)
find_file(LIB_UTILITY NAMES atutility.dll libatutility.so libatutility.so.*)

project(Andor3Capture)               
set(CMAKE_CXX_STANDARD 17)           

include_directories(include ${CMAKE_CURRENT_LIST_DIR})

add_executable(Andor3Capture main.cpp)
target_link_libraries(Andor3Capture ${LIB_CORE} ${LIB_UTILITY})