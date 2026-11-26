# Sandbox-Game

Custom engine written in C++ for simple 2d sandbox game, with mechanics inspired by Terraria.</br>
Uses CMake for build & Conan for dependencies.</br>
Uses custom RHI with OpenGL, Vulkan and DirectX 12 implementations.</br>
Uses custom UI widgets & system.</br>
Has built-in mods system.</br>
Has tools to pack data into binary files, generate SDF fonts and compile shaders to SPIR-V code.</br>
This engine should be considered as a pet-project.

# Main game mechanics & systems

1) Menu with UI
2) World Load/Save/Creation with options
3) World interaction (functional tiles, tiles place/destroy)
4) Inventory, HUD, Crafting
5) Player movement, tools & weapons usage
6) Entities with different behaviour
7) Mods Load/Reload
8) World smooth multicolor lighting 
...

# Used libraries

**GLFW - window, input/output**</br>
**GLM - math**</br>
**nlohmann_json - json serialization**</br>
**stb - image load/save**</br>
**miniaudio - sounds/music**</br>
**GLAD - OpenGL loader**</br>
**Vulkan headers & loader - Vulkan SDK**</br>
**VMA - Vulkan Memory Allocator**</br>
**SPIR-V headers & reflect - SPIR-V shaders reflect**</br>
**Shaderc - SPIR-V shaders compile**</br>
*msdfgen probably will be added later for MSDF text*

## License

This engine is distributed under the **MIT License**. See `LICENSE` for details.
All third-party libraries managed by Conan maintain their respective open-source licenses.