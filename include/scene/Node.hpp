#include<atomic>

namespace ENG {
	constexpr uint32_t HAS_DRAW_DATA{ 0x1 };
	constexpr uint32_t IS_VISIBLE{ 0x2 };
	constexpr uint32_t IS_A_CAMERA{ 0x4 };

	struct Node2 {
		uint32_t nodeId;  // index into "nodes" vector
		std::atomic_uint32_t propertyFlags;  // thread-safe if using util methods
	};

	inline static void set_property(Node2* node, const uint32_t propertyFlag);
	inline static bool has_property(const Node2* node, const uint32_t propertyFlag);
} // end namespace
