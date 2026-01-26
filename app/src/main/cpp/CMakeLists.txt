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
add_library(libcurl SHARED IMPORTED)
set_target_properties(libcurl PROPERTIES IMPORTED_LOCATION "${jnilibs}/${ANDROID_ABI}/libcurl.so")


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

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        lvgl-android

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        -landroid
        libcurl
        )