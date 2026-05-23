cmake_minimum_required(VERSION 3.10)
project(LuminaHUD LANGUAGES CXX)

cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)
set(IMGUI_BUILD_DIR ${CMAKE_SOURCE_DIR}/lib/imgui/backends)

file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui*.cpp"
    "${IMGUI_BUILD_DIR}/imgui_impl_glfw.cpp"
    "${IMGUI_BUILD_DIR}/imgui_impl_opengl3.cpp"
)

file(GLOB_RECURSE MY_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE MY_HEADERS "${CMAKE_SOURCE_DIR}/src/*.hpp")

set(MY_INCLUDE_DIRS "")
foreach(header_file ${MY_HEADERS})
    get_filename_component(header_dir ${header_file} DIRECTORY)
    list(APPEND MY_INCLUDE_DIRS ${header_dir})
endforeach()
list(REMOVE_DUPLICATES MY_INCLUDE_DIRS)

include_directories(
    ${IMGUI_DIR}
    ${IMGUI_BUILD_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${MY_INCLUDE_DIRS}
)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

add_executable(LuminaHUD
    ${MY_SOURCES}
    ${MY_HEADERS}
    ${IMGUI_SOURCES}
)

target_link_libraries(LuminaHUD
    PRIVATE
    glfw
    OpenGL::GL
    ${X11_LIBRARIES}
)

set_target_properties(LuminaHUD PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
