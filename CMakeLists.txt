# CMakeList.txt : CMake project for Arxetipo, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

message (${CMAKE_VERSION})

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Arxetipo")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Add libraries
find_package (Vulkan REQUIRED)
include_directories (${Vulkan_INCLUDE_DIRS})
add_subdirectory (external/OpenXR-SDK)
include_directories (external)
include_directories (external/OpenXR-SDK/include)
include_directories (external/OpenXR-Hpp/include)
include_directories (external/stb)

# Add subdirectories
add_subdirectory ("src")

# Add source to this project's executable.
add_executable (Arxetipo "src/Arxetipo.cpp" "src/Arxetipo.h")

target_link_libraries (Arxetipo ArxetipoEngine)
target_link_libraries (Arxetipo ${Vulkan_LIBRARIES})
target_link_libraries (Arxetipo openxr_loader)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Arxetipo PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
