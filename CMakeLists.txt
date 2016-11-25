cmake_minimum_required (VERSION 3.5)
project (vfpr)
set(CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/third_party/CMake/modules")

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

# Find our Vulkan!
find_package(Vulkan REQUIRED)


set(SOURCE_FILES
    "src/main.cpp"
    "src/third_party.cpp"
    "src/renderer/vulkan_util.h"
    "src/renderer/vulkan_util.cpp"
    "src/renderer/VDeleter.h"
    "src/renderer/VulkanShowBase.cpp"
    "src/renderer/VulkanShowBase.h"
    "src/util.h"
    "src/util.cpp"
    "src/Showbase.h"
    "src/Showbase.cpp"
    )

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

IF(MSVC)
    # make it looks better in msvc
    foreach(source IN LISTS SOURCE_FILES)
        get_filename_component(source_path "${source}" PATH)
        string(REPLACE "/" "\\" source_path_msvc "${source_path}")
        source_group("${source_path_msvc}" FILES "${source}")
    endforeach()
ENDIF(MSVC)

#target_link_libraries(${CMAKE_PROJECT_NAME} glfw ${GLFW_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} glfw)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${Vulkan_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
