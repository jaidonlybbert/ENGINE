#include "scene/Node.hpp"

void set_property(Node2& node, const property_t propertyFlag)
{
	node.propertyFlags |= propertyFlag;
}

bool has_property(const Node2& node, const property_t propertyFlag)
{
	return ((node.propertyFlags && propertyFlag) != 0);
}

void depth_first_traverse(node_array_t& nodes)
{
	/*
	for (auto& node : nodes) {
		nodeStack.push(node);
	}
	*/
};
