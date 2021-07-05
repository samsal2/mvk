cmake_minimum_required(VERSION 3.17.3)

project(mvk)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
        ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/main.cpp)

find_package(Vulkan REQUIRED)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/glm)
add_subdirectory(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} glfw) 
target_link_libraries(${PROJECT_NAME} glm::glm) 
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})
target_compile_options(${PROJECT_NAME} PRIVATE -O3 
                                               -Wall
                                               -Wextra
                                               -Werror
                                               -Wpedantic
                                               -pedantic-errors
                                               -Wshadow
                                               -fno-exceptions
                                               -fno-rtti
                                               )

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/)
