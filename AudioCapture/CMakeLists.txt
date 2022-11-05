cmake_minimum_required(VERSION 3.22)
project(VideoCapture)

set(CMAKE_CXX_STANDARD 14)

# 声明头文件位置
set(INCLUDE_DIR ../ffmpeg5.1/include)

# 声明链接库路径
set(LIBRARY_DIR ../ffmpeg5.1/lib)

# 引入头文件
include_directories(${INCLUDE_DIR})

# 引入库文件
link_directories(${LIBRARY_DIR})

add_executable(AudioCapture main.cpp)

# 引入三方库
target_link_libraries(AudioCapture avformat avcodec avdevice swscale)