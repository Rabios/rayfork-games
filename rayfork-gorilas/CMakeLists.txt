cmake_minimum_required(VERSION 3.1)
project(rayfork-gorilas LANGUAGES C) # Change the name to your project here

set(CMAKE_C_STANDARD 99)

# Setup GLAD
add_library(glad libs/glad/glad.c)
target_include_directories(glad PUBLIC libs)

# Setup rayfork
add_subdirectory(libs/rayfork)

# Setup project
add_executable(${CMAKE_PROJECT_NAME} src/platform.c src/gorilas.c)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glad rayfork)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE libs/rayfork libs/sokol)
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC)

# Enable MacOS specific flags and link against it's specific libraries
if (APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC "-framework Cocoa" "-framework QuartzCore" "-framework OpenGL")
    target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC "-fobjc-arc" "-ObjC")
endif()
