#pragma once
#include<cstdint>
#include<vector>

#include<glm/glm.hpp>
#include<glm/gtc/quaternion.hpp>

#include "renderer/vk_adapter/VkAdapter.hpp"

namespace ENG {
	using id_t = uint8_t;
	using property_t = uint16_t;

	// Size of statically allocated arrays
	constexpr size_t MAX_NUMBER_OF_NODES = 255;
	constexpr size_t MAX_NAME_LENGTH_CHARS = 16;

	// Property flags
	constexpr property_t HAS_DRAW_DATA = 0x1;
	constexpr property_t IS_VISIBLE = 0x1 << 1;
	constexpr property_t IS_A_CAMERA = 0x1 << 2;
	constexpr property_t IS_SELECTABLE = 0x1 << 3;
	constexpr property_t HAS_PHYSICS = 0x1 << 4;
	constexpr property_t HAS_SHADER = 0x1 << 5;
	constexpr property_t HAS_NAME = 0x1 << 6;
	constexpr property_t IS_SELECTED = 0x1 << 7;
	constexpr property_t IS_LAST_CHILD = 0x1 << 8;

	struct TRS {
		glm::vec3 scale{ 1.f };
		glm::vec3 translation{ 0.f };
		glm::quat rotation{ 1.f, 0.f, 0.f, 0.f };
	};

	struct Node2 {
		id_t                       nodeId{ 0 };  // index into "nodes" vector
		id_t                       parent{ 0 };
		property_t          propertyFlags{ 0 };
	};

	using name_t = std::array<char, MAX_NAME_LENGTH_CHARS>;
	using names_array_t = std::array<name_t, MAX_NUMBER_OF_NODES>;
	using node_array_t = std::array<Node2, MAX_NUMBER_OF_NODES>;
	using draw_data_array_t = std::array<DrawData, MAX_NUMBER_OF_NODES>;
	using trs_array_t = std::array<TRS, MAX_NUMBER_OF_NODES>;

	names_array_t     names;
	node_array_t      nodes;
	draw_data_array_t drawData;
	trs_array_t       trs;

	inline static void set_property(Node2& node, const property_t propertyFlag);
	inline static bool has_property(const Node2& node, const property_t propertyFlag);
	inline static void depth_first_traverse(node_array_t& nodes);
}
