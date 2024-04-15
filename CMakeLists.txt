cmake_minimum_required(VERSION 3.13) 
project(Andor3Capture)               
set(CMAKE_CXX_STANDARD 17)           

include_directories(include ${CMAKE_CURRENT_LIST_DIR})

add_executable(Andor3Capture main.cpp)
target_link_libraries(Andor3Capture "C:\\Program Files\\Andor SDK3\\atcore.dll")