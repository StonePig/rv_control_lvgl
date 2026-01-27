# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("lvgl-android")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

file(GLOB_RECURSE ALL_SOURCE "*.cpp" "*.c" "*.h")

file(GLOB_RECURSE ALL_INCLUDE "*.h")

include_directories(${ALL_INCLUDE})

# set(jnilibs E:/git/rv_control_lvgl/app/src/main/jniLibs)
set(jnilibs "${CMAKE_SOURCE_DIR}/jniLibs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${jnilibs}/${ANDROID_ABI})

add_library( # Sets the name of the library.
        lvgl-android

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${ALL_SOURCE})

# 引入 .so 文件
add_library(libcurl STATIC IMPORTED)
set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION "${jnilibs}/${ANDROID_ABI}/libcurl.a")

# 引入 libssl 静态库
add_library(libssl STATIC IMPORTED)
set_target_properties(libssl PROPERTIES IMPORTED_LOCATION "${jnilibs}/${ANDROID_ABI}/libssl.a")

# 引入 libcrypto 静态库
add_library(libcrypto STATIC IMPORTED)
set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION "${jnilibs}/${ANDROID_ABI}/libcrypto.a")


# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# 查找标准C库
find_library(
        c-lib
        c)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

# 设置链接器标志，强制链接标准C库的符号
# 在 Android NDK 中，需要明确指定标准C库
set_target_properties(lvgl-android PROPERTIES
    LINK_FLAGS "-Wl,-u,stdout -Wl,-u,stderr -Wl,-u,stdin -lc"
)

target_link_libraries( # Specifies the target library.
        lvgl-android

        # 先链接静态库（使用 --whole-archive 确保所有符号都被包含）
        -Wl,--whole-archive
        libcurl
        libssl
        libcrypto
        -Wl,--no-whole-archive
        
        # 然后链接系统库（包括标准C库，提供 stdout/stderr 等符号）
        # 注意：系统库必须在静态库之后链接，以便解析静态库中的未定义符号
        ${log-lib}
        ${c-lib}
        -landroid
        -lz
        -lm
        -ldl
        # 最后再次链接标准C库，确保所有符号都被解析
        -lc
        )