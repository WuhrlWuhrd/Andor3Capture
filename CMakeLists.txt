cmake_minimum_required(VERSION 3.13) 

project(Andor3Capture)               
set(CMAKE_CXX_STANDARD 17)
include_directories(include ${CMAKE_CURRENT_LIST_DIR})
include_directories(include ${CMAKE_CURRENT_LIST_DIR}/src)

if(UNIX)

    find_file(LIB_CORE NAMES libatcore.so libatcore.so.3.10.30003.5 libatcore.so.3.15.30092.2 HINTS ${CMAKE_CURRENT_LIST_DIR})
    find_file(LIB_UTILITY NAMES libatutility.so HINTS ${CMAKE_CURRENT_LIST_DIR})

else()

    find_file(LIB_CORE NAMES atcore.dll HINTS ${CMAKE_CURRENT_LIST_DIR})
    find_file(LIB_UTILITY NAMES atutility.dll HINTS ${CMAKE_CURRENT_LIST_DIR})

endif()

# If SWIG is available, use it to generate the PyAndor3Capture python wrapper
find_package(SWIG 4.0 COMPONENTS python)

if(SWIG_FOUND)

    cmake_policy(SET CMP0086 NEW)

    message("SWIG found: ${SWIG_EXECUTABLE}")
    INCLUDE(${SWIG_USE_FILE})

    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    include_directories(${Python3_INCLUDE_DIRS})

    SET_SOURCE_FILES_PROPERTIES(src/Zyla.i PROPERTIES CPLUSPLUS ON)
    SET_SOURCE_FILES_PROPERTIES(src/Zyla.i PROPERTIES SWIG_FLAGS "-includeall")
    SET_SOURCE_FILES_PROPERTIES(src/Zyla.i PROPERTIES OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/build/PyZyla)

    set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_LIST_DIR}/build/PyZyla)
    set_property(SOURCE src/Zyla.i PROPERTY CPLUSPLUS ON)

    swig_add_library(PyZyla LANGUAGE python SOURCES src/Zyla.i ${SOURCES})

    if(LIB_UTILITY_FOUND)
        swig_link_libraries(PyZyla ${LIB_CORE} ${LIB_UTILITY})
    else()
        swig_link_libraries(PyZyla ${LIB_CORE})
    endif()

    configure_file(src/python/zylaui.py ${CMAKE_SWIG_OUTDIR}/zylaui.py COPYONLY)
    configure_file(src/python/dataStream.ui ${CMAKE_SWIG_OUTDIR}/dataStream.ui COPYONLY)

    set_target_properties(PyZyla PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})

else()
    message("SWIG NOT FOUND! No Python module will be compiled as a result.")
endif()         

add_executable(Andor3Capture src/main.cpp)
add_library(Zyla SHARED src/Zyla.cpp)

if(LIB_UTILITY_FOUND)
    target_link_libraries(Andor3Capture ${LIB_CORE} ${LIB_UTILITY})
else()
    target_link_libraries(Andor3Capture ${LIB_CORE})
    add_compile_definitions(NO_LIB_UTILITY)
endif()