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

# Set Binary Tree
set (ShaderOutput ${CMAKE_CURRENT_BINARY_DIR}/shaders)

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

# Compile shaders
file(GLOB_RECURSE GLSL_SHADER_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert" # vertex shaders 
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag" # fragment shaders
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp" # compute shaders
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom" # geometry shaders
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tesc" # tessellation control shaders
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tese" # tessellation evaluation shaders
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.mesh" # mesh shaders
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.task" # task shaders
)

find_program (VULKAN_GLSL_VALIDATOR glslangValidator HINTS
	${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
)

message (${VULKAN_GLSL_VALIDATOR})

foreach (GLSL_SHADER ${GLSL_SHADER_FILES})
	get_filename_component (FILE_NAME ${GLSL_SHADER} NAME)
	get_filename_component (FILE_DIR ${GLSL_SHADER} DIRECTORY)
	file (RELATIVE_PATH REL_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${FILE_DIR})
	# file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${REL_DIR})
	set (SPIRV_SHADER ${CMAKE_CURRENT_BINARY_DIR}/${REL_DIR}/${FILE_NAME}.spv)
	add_custom_command (
		OUTPUT ${SPIRV_SHADER}
		COMMAND ${VULKAN_GLSL_VALIDATOR} -V ${GLSL_SHADER} -o ${SPIRV_SHADER}
		DEPENDS ${GLSL_SHADER}
		COMMENT "Compiling shader ${GLSL_SHADER} to ${SPIRV_SHADER}"
	)
	message ("${VULKAN_GLSL_VALIDATOR} -V ${GLSL_SHADER} -o ${SPIRV_SHADER}")
	list(APPEND SPIRV_BINARY_FILES ${SPIRV_SHADER})
endforeach (GLSL_SHADER)

add_custom_target (Shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(Arxetipo Shaders)

# TODO: Add tests and install targets if needed.
