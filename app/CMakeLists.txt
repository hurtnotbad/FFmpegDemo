# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

include_directories(libs)
include_directories(libs/include)
include_directories(libs/libyuv)

add_library( # Sets the name of the library.
             native-lib


             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp
             src/main/cpp/FFmpegVideoPlayer.cpp
             src/main/cpp/IFFmpegVideoPlayer.cpp
             )


set(distribution_DIR ${CMAKE_SOURCE_DIR}/libs/)

#libavcodec-56.so
add_library(libavcodec-57-lib SHARED IMPORTED)
set_target_properties(libavcodec-57-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libavcodec-57.so)


#libavdevice-57.so
add_library(libavdevice-57-lib SHARED IMPORTED)
set_target_properties(libavdevice-57-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libavdevice-57.so)

#libavfilter-6.so
add_library(libavfilter-6-lib SHARED IMPORTED)
set_target_properties(libavfilter-6-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libavfilter-6.so)

#libavformat-57.so
add_library(libavformat-57-lib SHARED IMPORTED)
set_target_properties(libavformat-57-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libavformat-57.so)

#libavutil-54.so
add_library(libavutil-55-lib SHARED IMPORTED)
set_target_properties(libavutil-55-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libavutil-55.so)


#libpostproc-54.so
add_library(libpostproc-54-lib SHARED IMPORTED)
set_target_properties(libpostproc-54-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libpostproc-54.so)


#libswresample-2.so
add_library(libswresample-2-lib SHARED IMPORTED)
set_target_properties(libswresample-2-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libswresample-2.so)


#libswscale-4.so
add_library(libswscale-4-lib SHARED IMPORTED)
set_target_properties(libswscale-4-lib  PROPERTIES IMPORTED_LOCATION
${distribution_DIR}/${ANDROID_ABI}/libswscale-4.so)

#libyuv.so
add_library(libyuv-lib SHARED IMPORTED)
set_target_properties(libyuv-lib PROPERTIES IMPORTED_LOCATION ${distribution_DIR}/${ANDROID_ABI}/libyuv.so)



##################################################################################################
##################################################################################################



# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )
              #这个库是用于surface在c代码中交互而添加的
find_library( # Sets the name of the path variable.
              lnative_window_jni-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              android )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
include_directories(src/main/cpp/include/)
target_link_libraries( # Specifies the target library.
                       native-lib

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       ${lnative_window_jni-lib}
                       libavcodec-57-lib
                       libavdevice-57-lib
                       libavfilter-6-lib
                       libavformat-57-lib
                       libavutil-55-lib
                       libpostproc-54-lib
                       libswresample-2-lib
                       libswscale-4-lib
                       libyuv-lib
                        )