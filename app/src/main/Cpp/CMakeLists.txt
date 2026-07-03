# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.22.1)

# Declares and names the project.
project("backgroundchange")

# Specifies the native library name, its type (SHARED), 
# and provides the relative path to your source file (main.cpp).
add_library(
        # This matches the library name that MainActivity.java attempts to load
        backgroundchange
        SHARED
        main.cpp
)

# Searches for a specified prebuilt library and stores the path as a variable.
# The 'log' library is an Android-specific library used to print logs to Logcat.
find_library(
        log-lib
        log
)

# Specifies the libraries CMake should link to your target native library.
target_link_libraries(
        # The library you created above
        backgroundchange
        
        # Links the Android Log library so your C++ code can print log outputs
        ${log-lib}
)
