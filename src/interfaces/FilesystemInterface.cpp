#include<filesystem>
#include "interfaces/FilesystemInterface.hpp"
#include "EngineConfig.hpp"

namespace ENG
{

const std::filesystem::path& get_install_dir() {
	static const std::filesystem::path& install_dir{Engine_INSTALL_DIR};
	return install_dir;
}

const std::filesystem::path& get_model_dir() {
	static const std::filesystem::path& model_path{get_install_dir() / "models" / "viking_room.obj"};
	return model_path;
}

const std::filesystem::path& get_tex_path() {
	static const std::filesystem::path& tex_path{ get_install_dir() / "textures" / "viking_room.png"};
	return tex_path;
}

const std::filesystem::path& get_gltf_dir() {
	static const std::filesystem::path& gltf_dir{ get_install_dir() / "gltf" / "suzanne" / "suzanne.gltf" };
	return gltf_dir;
}

}
