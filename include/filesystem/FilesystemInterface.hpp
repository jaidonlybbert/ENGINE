#ifndef ENG_FILESYSTEM_INTERFACE
#define ENG_FILESYSTEM_INTERFACE
#include<filesystem>

namespace ENG
{
const std::filesystem::path& get_install_dir();
const std::filesystem::path& get_model_dir();
const std::filesystem::path& get_tex_path();
const std::filesystem::path& get_gltf_dir();
}
#endif
