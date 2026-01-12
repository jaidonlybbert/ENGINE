#include "scene/Node.hpp"

namespace ENG {
	inline static void set_property(Node2* node, const uint32_t propertyFlag)
	{
		node->propertyFlags = node->propertyFlags.fetch_or(propertyFlag);
	}

	inline static bool has_property(const Node2* node, const uint32_t propertyFlag)
	{
		return ((node->propertyFlags && propertyFlag) != 0);
	}
} // end namespace
