cmake_minimum_required(VERSION 3.10)
project(ProcessWindow LANGUAGES CXX)

# Supprime le warning OpenGL en spécifiant qu'on préfère GLVND (moderne)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)
set(IMGUI_BUILD_DIR ${CMAKE_SOURCE_DIR}/lib/imgui/backends)

# ImGui sources
file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui*.cpp"
    "${IMGUI_BUILD_DIR}/imgui_impl_glfw.cpp"
    "${IMGUI_BUILD_DIR}/imgui_impl_opengl3.cpp"
)

# 1. Trouve TOUS tes fichiers .cpp et .hpp (cherche partout dans les sous-dossiers)
file(GLOB_RECURSE MY_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE MY_HEADERS "${CMAKE_SOURCE_DIR}/src/*.hpp")

# 2. Détecte automatiquement tous les sous-dossiers qui contiennent un fichier .hpp
set(MY_INCLUDE_DIRS "")
foreach(header_file ${MY_HEADERS})
    get_filename_component(header_dir ${header_file} DIRECTORY)
    list(APPEND MY_INCLUDE_DIRS ${header_dir})
endforeach()
list(REMOVE_DUPLICATES MY_INCLUDE_DIRS) # Nettoie la liste pour éviter les doublons

# 3. Inclusion des dossiers pour ImGui et TOUS tes sous-dossiers automatiques
include_directories(
    ${IMGUI_DIR}
    ${IMGUI_BUILD_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${MY_INCLUDE_DIRS} # <- Tous tes sous-sous-dossiers de src/ sont inclus ici !
)

# Find Packages
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

# Add executable
add_executable(ProcessWindow
    ${MY_SOURCES}
    ${MY_HEADERS}
    ${IMGUI_SOURCES}
)

# Link libraries
target_link_libraries(ProcessWindow
    PRIVATE
    glfw
    OpenGL::GL
    ${X11_LIBRARIES}
)

# Set output directory for executable
set_target_properties(ProcessWindow PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
