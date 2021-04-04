# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)


file(GLOB source_file_cpp src/main/cpp/*.cpp)
file(GLOB source_file_head src/main/cpp/*.h)


#添加头文件
include_directories(src/main/cpp/include)

include_directories(sysroot/usr/include)
#添加库文件
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/src/main/cpp/libs/${CMAKE_ANDROID_ARCH_ABI}")


add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED


       ${source_file_cpp}
       ${source_file_head}


        )


find_library(
        log-lib
        log)


target_link_libraries(
        native-lib
        -Wl,--start-group # 忽略顺序引发的问题
        avcodec avfilter avformat avutil swresample swscale
        ${log-lib}
        z
        android #使用nativeWindow.h编译必须链接到
        OpenSLES #使用native播放音频原始数据PCM
)