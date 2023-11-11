cmake_minimum_required(VERSION 3.14)

project(ffmpeg_test LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/include")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/lib")

add_executable(ffmpeg_test
  main.cpp
)
target_link_libraries(ffmpeg_test Qt${QT_VERSION_MAJOR}::Core Qt6::Gui swresample;postproc;avutil;avcodec;avformat;avdevice;avfilter;swscale;avdevice)
#target_link_libraries(ffmpeg_test PRIVATE avutil;avcodec;avformat;avdevice;avfilter;swscale;avresample)

install(TARGETS ffmpeg_test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


