cmake_minimum_required(VERSION 3.25)
project(yzw_text)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(detector)
set(CMAKE_CXX_STANDARD 11)

add_executable(yzw_text
        main.cpp
        detector/detector.cpp
        detector/detector.h
        Camera/GXcamera.cpp
        Camera/GXcamera.h
        Camera/include/DxImageProc.h
        Camera/include/GxIAPI.h)

target_link_libraries(yzw_text ${OpenCV_LIBS}
        -lpthread
        -lm
        -lgxiapi)