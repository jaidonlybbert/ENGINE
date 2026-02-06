#ifndef ENG_FILESYSTEM_INTERFACE
#define ENG_FILESYSTEM_INTERFACE
#include<filesystem>

namespace ENG
{
const std::filesystem::path& get_install_dir();
const std::filesystem::path& get_gltf_dir();
const std::filesystem::path& get_mtl_dir();

const std::filesystem::path& get_room_obj();
const std::filesystem::path& get_room_tex();
const std::filesystem::path& get_spacefloor_obj();
const std::filesystem::path& get_spacefloor_obj2();
const std::filesystem::path& get_spacefloor_tex();
}
#endif
