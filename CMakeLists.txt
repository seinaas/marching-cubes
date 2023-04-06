cmake_minimum_required(VERSION 3.20)

project(marching_cubes) # The name of your choice for the project comes here

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_PREFIX_PATH
    /encs # For ENCS lab computers
    /opt/local # Macports
    /opt/homebrew
)

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED
    HINTS /encs/pkg/glfw-3.3.4/root # ENCS installation of glfw
)

# NOTE: ENCS glm installation is missing links to *.inl files so we need this line
include_directories(/encs/pkg/glm-0.9.9.8/root/include)

include_directories(/src)
include_directories(/external)

aux_source_directory(src SOURCE)
aux_source_directory(external SOURCE)

add_executable(${PROJECT_NAME} main.cpp ${SOURCE}) #The name of the cpp file and its path can vary

set(IMGUI_PATH  ${CMAKE_CURRENT_LIST_DIR}/external/imgui)
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp ${IMGUI_PATH}/backends/*.cpp)
message(STATUS "IMGUI_SOURCES: ${IMGUI_SOURCES}")
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})

target_link_libraries(${PROJECT_NAME} ImGui OpenGL::GL GLEW::glew glfw)
target_link_libraries(ImGui glfw)

add_custom_target(copy_shaders ALL 
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_BINARY_DIR}/shaders
)

add_dependencies(${PROJECT_NAME} copy_shaders)