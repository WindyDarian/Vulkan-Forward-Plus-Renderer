cmake_minimum_required (VERSION 3.5)
project (vulkan_helloworld)
set(CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/third_party/CMake/modules")

set(SOURCE_FILES
    "src/main.cpp"
    "src/VDeleter.h"
    "src/VulkanShowBase.cpp"
    "src/VulkanShowBase.h"
    )

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

set(EXTERNAL "third_party")

include_directories("${EXTERNAL}/include")

# Configure GLFW
set(GLFW_ROOT_DIR "${EXTERNAL}/glfw-3.2.1")
#set(GLFW_ROOT_DIR "${EXTERNAL}/glfw")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_ROOT_DIR})
include_directories("${GLFW_ROOT_DIR}/include")
#target_link_libraries(${CMAKE_PROJECT_NAME} glfw ${GLFW_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} glfw)

find_package(Vulkan REQUIRED)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${Vulkan_LIBRARIES})
