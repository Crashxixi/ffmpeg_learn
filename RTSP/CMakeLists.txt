cmake_minimum_required(VERSION 3.22)
project(rtsp)

set(CMAKE_CXX_STANDARD 14)

set(INCLUDE_DIR ../ffmpeg5.1/include)

set(LIBRARY_DIR ../ffmpeg5.1/lib)

set(CMAKE_BUILD_TYPE Debug)

include_directories(${INCLUDE_DIR})

link_directories(${LIBRARY_DIR})

add_executable(rtsp main.cpp logger.h common.h CommonLooper.cpp CommonLooper.h VideoCapture.cpp VideoCapture.h VideoEncode.cpp VideoEncode.h PushWorker.cpp PushWorker.h AudioCapture.cpp AudioCapture.h AudioEncode.cpp AudioEncode.h ffmpeg.h RtspPusher.cpp RtspPusher.h PacketQueue.cpp PacketQueue.h PushTime.cpp PushTime.h)

target_link_libraries(rtsp avformat avdevice avcodec swscale)