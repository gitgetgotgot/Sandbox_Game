cmake_minimum_required(VERSION 3.30)
project(OpenGL_2D_Sandbox VERSION 1.0)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/EngineModule.cmake)
include(cmake/EngineInterfaceModule.cmake)

add_engine_interface_module(Base)
add_engine_module(Audio)
add_engine_module(Core)
add_engine_module(Entities)
add_engine_module(Objects)
add_engine_module(Physics)
add_engine_module(Rendering)
add_engine_module(Resources)
add_engine_module(UI)
add_engine_module(DeviceSystem)
add_engine_module(Utility)
add_engine_module(EngineAPI)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(stb REQUIRED)
find_package(miniaudio REQUIRED)

find_package(glad REQUIRED)

find_package(VulkanLoader REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(SPIRV-Headers REQUIRED)
find_package(spirv-reflect-static REQUIRED)

target_link_libraries(DeviceSystem
    PUBLIC
        glm::glm
    PRIVATE
        glfw
        glad::glad
        Base
        Utility
        Rendering
)

target_link_libraries(Utility
    PUBLIC
        glm::glm
        nlohmann_json::nlohmann_json
    PRIVATE
        Base
        stb::stb
)

target_link_libraries(Rendering
    PUBLIC
        glm::glm
        Vulkan::Loader
        vulkan-headers::vulkan-headers
        GPUOpen::VulkanMemoryAllocator
    PRIVATE
	    SPIRV-Headers::SPIRV-Headers
        spirv-reflect-static
        glad::glad
        stb::stb
        Utility
        Base
)

if(WIN32)
    target_link_libraries(Rendering PRIVATE d3d12 dxgi dxguid d3dcompiler)
endif()

target_link_libraries(Resources
    PUBLIC
        Utility
        Base
    PRIVATE
        stb::stb
        Rendering
)

target_link_libraries(Audio
    PUBLIC
        miniaudio::miniaudio
    PRIVATE
        Utility
)

target_link_libraries(Physics
    PUBLIC
        glm::glm
    PRIVATE
        Base 
)

target_link_libraries(UI
    PUBLIC
        Rendering
        Resources
    PRIVATE
        DeviceSystem
)

target_link_libraries(Objects
    PUBLIC
        Physics
        Resources
)

target_link_libraries(Entities
    PUBLIC
        Objects
    PRIVATE
        DeviceSystem
)

target_link_libraries(Core
    PUBLIC
        UI
        Objects
        Entities
        Audio
    PRIVATE
        DeviceSystem
)

target_link_libraries(EngineAPI PUBLIC Entities)

target_compile_definitions(Utility PUBLIC
    $<$<CONFIG:Debug>:CMAKE_DEBUG_MACRO=1>
    $<$<CONFIG:Release>:CMAKE_DEBUG_MACRO=1>
)


add_executable(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE Core)

# Shader Compiler for Vulkan
add_subdirectory(Tools/ShaderCompiler)

add_custom_target(CompileShaders ALL #include this target to default build
    COMMAND $<TARGET_FILE:ShaderCompiler>
            "${PROJECT_SOURCE_DIR}/Resources/shaders/GLSL/Common"
            "${PROJECT_SOURCE_DIR}/Resources/shaders/SPIRV/Common"
            "$<IF:$<CONFIG:Debug>,debug,release>"
    COMMAND $<TARGET_FILE:ShaderCompiler>
            "${PROJECT_SOURCE_DIR}/Resources/shaders/GLSL/VK_Specific"
            "${PROJECT_SOURCE_DIR}/Resources/shaders/SPIRV/VK_Specific"
            "$<IF:$<CONFIG:Debug>,debug,release>"
    DEPENDS ShaderCompiler #ShaderCompiler compile first
    COMMENT "Compiling GLSL shaders to SPIR-V..."
)

add_dependencies(${PROJECT_NAME} ShaderCompiler)

# Copy data folders
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources"
    COMMAND ${CMAKE_COMMAND} -E 
        $<IF:$<CONFIG:Debug>,create_symlink,copy_directory>
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources"
    COMMAND ${CMAKE_COMMAND} -E echo "Resources successfully configured for $<CONFIG> mode."
    COMMENT "Configuring Resources (Symlink for Debug, Copy for Release)."
)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Config"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Config"
    COMMENT "Copy Config folder to build folder."
)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Mods"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Mods"
    COMMENT "Copy Mods folder to build folder."
)